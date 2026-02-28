import 'dart:async';

import 'package:flutter_riverpod/flutter_riverpod.dart';

import '../data/repositories/client_repository.dart';
import '../data/repositories/device_repository.dart';
import '../models/device_command.dart';
import '../models/switch_module.dart';

class CommandRateLimitException implements Exception {
  final int waitMs;

  const CommandRateLimitException(this.waitMs);
}

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
  static const int _confirmWindowMs = 1300;
  static const int _commandRateLimitMs = 1200;

  final String clientId;
  final ClientRepository clientRepository;
  final DeviceRepository deviceRepository;

  final Map<String, _PendingTarget> _pendingTargets = {};
  final Map<String, Timer> _pendingTimers = {};
  final Map<String, Timer> _rateLimitTimers = {};
  final Map<String, DateTime> _lastCommandAt = {};
  final Set<String> _staleComps = <String>{};
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
    bool refresh = false,
    bool silent = false,
  }) async {
    try {
      final snapshot = await deviceRepository.requestStatus(
        serverId: serverId,
        devId: clientId,
        comp: compId,
        refresh: refresh,
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
    final waitMs = _remainingRateLimitMs(compId);
    if (waitMs > 0) {
      throw CommandRateLimitException(waitMs);
    }
    _markRateLimited(compId);

    _markPending(compId, mode, status, value);
    _clearStale(compId);
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
      _scheduleConfirmationFallback(serverId, compId);
    } catch (_) {
      _clearPending(compId);
      _markStale(compId);
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
      if (!pending.matches(mode, status, value) &&
          pending.ageMs < _confirmWindowMs) {
        return;
      }
      _clearPending(compId, notify: false);
    }

    _clearStale(compId, notify: false);
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

    bool changedMeta = false;
    for (final module in modules) {
      final pending = _pendingTargets[module.compId];
      if (pending != null) {
        if (pending.matches(module.mode, module.status, module.value)) {
          _clearPending(module.compId, notify: false);
          if (_staleComps.remove(module.compId)) {
            changedMeta = true;
          }
          _upsertModule(module);
        }
        continue;
      }

      if (_staleComps.remove(module.compId)) {
        changedMeta = true;
      }
      _upsertModule(module);
    }

    if (changedMeta) {
      _notifyStateUnchanged();
    }
  }

  bool isPending(String compId) => _pendingTargets.containsKey(compId);

  bool isStale(String compId) => _staleComps.contains(compId);

  bool isRateLimited(String compId) => _remainingRateLimitMs(compId) > 0;

  int rateLimitRemainingMs(String compId) => _remainingRateLimitMs(compId);

  void _markPending(String compId, int mode, int status, int value) {
    _pendingTargets[compId] = _PendingTarget(
      mode: mode,
      status: status,
      value: value,
      since: DateTime.now(),
    );
    _notifyStateUnchanged();
  }

  void _markRateLimited(String compId) {
    final now = DateTime.now();
    _lastCommandAt[compId] = now;
    _rateLimitTimers[compId]?.cancel();
    _rateLimitTimers[compId] = Timer(
      const Duration(milliseconds: _commandRateLimitMs),
      _notifyStateUnchanged,
    );
    _notifyStateUnchanged();
  }

  int _remainingRateLimitMs(String compId) {
    final last = _lastCommandAt[compId];
    if (last == null) {
      return 0;
    }
    final elapsed = DateTime.now().difference(last).inMilliseconds;
    final remaining = _commandRateLimitMs - elapsed;
    return remaining > 0 ? remaining : 0;
  }

  void _scheduleConfirmationFallback(String serverId, String compId) {
    _pendingTimers[compId]?.cancel();
    _pendingTimers[compId] = Timer(
      const Duration(milliseconds: _confirmWindowMs),
      () {
        unawaited(_runFallbackRefresh(serverId, compId));
      },
    );
  }

  Future<void> _runFallbackRefresh(String serverId, String compId) async {
    if (!isPending(compId)) {
      return;
    }

    await refreshFromDevice(
      serverId,
      compId: compId,
      refresh: true,
      silent: true,
    );

    if (!isPending(compId)) {
      return;
    }

    _clearPending(compId);
    _markStale(compId);
  }

  void _notifyStateUnchanged() {
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

  void _clearPending(String compId, {bool notify = true}) {
    _pendingTargets.remove(compId);
    _pendingTimers.remove(compId)?.cancel();
    if (notify) {
      _notifyStateUnchanged();
    }
  }

  void _markStale(String compId, {bool notify = true}) {
    if (_staleComps.add(compId) && notify) {
      _notifyStateUnchanged();
    }
  }

  void _clearStale(String compId, {bool notify = true}) {
    if (_staleComps.remove(compId) && notify) {
      _notifyStateUnchanged();
    }
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
    for (final timer in _rateLimitTimers.values) {
      timer.cancel();
    }
    _rateLimitTimers.clear();
    super.dispose();
  }
}
