import 'dart:async';

import 'package:flutter_riverpod/flutter_riverpod.dart';

import '../data/repositories/client_repository.dart';
import '../data/repositories/device_repository.dart';
import '../models/device_command.dart';
import '../models/switch_module.dart';

class _PendingTarget {
  final int mode;
  final int status;
  final int value;
  final DateTime since;

  const _PendingTarget({
    required this.mode,
    required this.status,
    required this.value,
    required this.since,
  });

  bool matches(int incomingMode, int incomingStatus, int incomingValue) {
    return mode == incomingMode &&
        status == incomingStatus &&
        value == incomingValue;
  }

  int get ageMs => DateTime.now().difference(since).inMilliseconds;
}

class SwitchModulesController
    extends StateNotifier<AsyncValue<List<SwitchModule>>> {
  final String clientId;
  final ClientRepository clientRepository;
  final DeviceRepository deviceRepository;

  final Map<String, _PendingTarget> _pendingTargets = {};
  final Map<String, Timer> _pendingTimers = {};
  final Map<String, int> _pendingAttempts = {};
  final Map<String, int> _lastTs = {};

  SwitchModulesController({
    required this.clientId,
    required this.clientRepository,
    required this.deviceRepository,
  }) : super(const AsyncValue.loading()) {
    load();
  }

  Future<void> load({bool showLoading = true}) async {
    if (showLoading || state.value == null) {
      state = const AsyncValue.loading();
    }
    try {
      final modules = await clientRepository.listModules(clientId);
      state = AsyncValue.data(modules);
    } catch (error, stack) {
      if (showLoading || state.value == null) {
        state = AsyncValue.error(error, stack);
      }
    }
  }

  Future<void> refreshFromDevice(
    String serverId, {
    String? compId,
    bool silent = false,
  }) async {
    try {
      final snapshot = await deviceRepository.requestStatus(
        serverId: serverId,
        devId: clientId,
        comp: compId,
      );
      applyServerSnapshot(snapshot);
    } catch (_) {
      if (!silent) {
        rethrow;
      }
    }
  }

  Future<void> sendCommand({
    required String serverId,
    required String compId,
    required int mode,
    required int status,
    required int value,
  }) async {
    _markPending(compId, mode, status, value);
    _applyOptimistic(compId, mode, status, value);
    try {
      await deviceRepository.sendCommand(
        DeviceCommand(
          serverId: serverId,
          devId: clientId,
          comp: compId,
          mod: mode,
          stat: status,
          val: value,
        ),
      );
      _schedulePendingReconcile(serverId, compId);
    } catch (_) {
      _clearPending(compId);
      unawaited(refreshFromDevice(serverId, compId: compId, silent: true));
      rethrow;
    }
  }

  void applySocketUpdate(Map<String, dynamic> data) {
    final devId = data['devID']?.toString();
    final compId = data['comp']?.toString();
    if (devId == null || compId == null) {
      return;
    }

    final normalized = _normalizeId(devId);
    if (normalized != clientId) {
      return;
    }

    final mode = (data['mod'] as num?)?.toInt() ?? 0;
    final status = (data['stat'] as num?)?.toInt() ?? 0;
    final value = (data['val'] as num?)?.toInt() ?? 0;
    final ts = (data['ts'] as num?)?.toInt();
    if (ts != null) {
      final lastTs = _lastTs[compId];
      if (lastTs != null && ts < lastTs) {
        return;
      }
      if (lastTs != null && ts == lastTs) {
        final current = _findModule(compId);
        if (current != null &&
            current.mode == mode &&
            current.status == status &&
            current.value == value) {
          return;
        }
      }
      _lastTs[compId] = ts;
    }

    final pending = _pendingTargets[compId];
    if (pending != null) {
      final confirmed = pending.matches(mode, status, value);
      if (!confirmed && pending.ageMs < 1500) {
        return;
      }
      _clearPending(compId);
    }

    _upsertModule(
      SwitchModule(
        clientId: clientId,
        compId: compId,
        mode: mode,
        status: status,
        value: value,
      ),
    );
  }

  void applyServerSnapshot(List<SwitchModule> modules) {
    if (modules.isEmpty) {
      return;
    }
    for (final module in modules) {
      final pending = _pendingTargets[module.compId];
      if (pending != null) {
        final confirmed = pending.matches(
          module.mode,
          module.status,
          module.value,
        );
        if (confirmed || pending.ageMs >= 4500) {
          _clearPending(module.compId);
        }
      }
      _upsertModule(module);
    }
  }

  bool isPending(String compId) => _pendingTargets.containsKey(compId);

  void _markPending(String compId, int mode, int status, int value) {
    _pendingTargets[compId] = _PendingTarget(
      mode: mode,
      status: status,
      value: value,
      since: DateTime.now(),
    );
    _notifyPending();
  }

  void _schedulePendingReconcile(String serverId, String compId) {
    _pendingTimers[compId]?.cancel();
    _pendingAttempts[compId] = 0;
    _schedulePendingTick(serverId, compId, const Duration(milliseconds: 700));
  }

  void _schedulePendingTick(String serverId, String compId, Duration delay) {
    _pendingTimers[compId]?.cancel();
    _pendingTimers[compId] = Timer(delay, () {
      unawaited(_runPendingTick(serverId, compId));
    });
  }

  Future<void> _runPendingTick(String serverId, String compId) async {
    if (!isPending(compId)) {
      return;
    }

    await refreshFromDevice(serverId, compId: compId, silent: true);
    if (!isPending(compId)) {
      return;
    }

    final attempts = (_pendingAttempts[compId] ?? 0) + 1;
    _pendingAttempts[compId] = attempts;
    if (attempts >= 3) {
      _clearPending(compId);
      await load(showLoading: false);
      return;
    }

    if (attempts == 1) {
      _schedulePendingTick(
        serverId,
        compId,
        const Duration(milliseconds: 1200),
      );
      return;
    }
    _schedulePendingTick(serverId, compId, const Duration(milliseconds: 2200));
  }

  void _notifyPending() {
    final modules = state.value;
    if (modules == null) {
      return;
    }
    state = AsyncValue.data(List<SwitchModule>.from(modules));
  }

  void _applyOptimistic(String compId, int mode, int status, int value) {
    _upsertModule(
      SwitchModule(
        clientId: clientId,
        compId: compId,
        mode: mode,
        status: status,
        value: value,
      ),
    );
  }

  void _upsertModule(SwitchModule incoming) {
    final modules = List<SwitchModule>.from(
      state.value ?? const <SwitchModule>[],
    );
    final index = modules.indexWhere(
      (module) => module.compId == incoming.compId,
    );
    if (index >= 0) {
      modules[index] = incoming;
    } else {
      modules.add(incoming);
    }
    state = AsyncValue.data(modules);
  }

  SwitchModule? _findModule(String compId) {
    final modules = state.value;
    if (modules == null) {
      return null;
    }
    for (final module in modules) {
      if (module.compId == compId) {
        return module;
      }
    }
    return null;
  }

  void _clearPending(String compId) {
    _pendingTargets.remove(compId);
    _pendingAttempts.remove(compId);
    _pendingTimers.remove(compId)?.cancel();
    _notifyPending();
  }

  String _normalizeId(String devId) {
    if (devId.startsWith('RSW-')) {
      return devId;
    }
    return 'RSW-$devId';
  }

  @override
  void dispose() {
    for (final timer in _pendingTimers.values) {
      timer.cancel();
    }
    _pendingTimers.clear();
    super.dispose();
  }
}
