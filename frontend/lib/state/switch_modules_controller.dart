import 'package:flutter_riverpod/flutter_riverpod.dart';

import '../data/repositories/client_repository.dart';
import '../data/repositories/device_repository.dart';
import '../models/device_command.dart';
import '../models/switch_module.dart';

class SwitchModulesController
    extends StateNotifier<AsyncValue<List<SwitchModule>>> {
  final String clientId;
  final ClientRepository clientRepository;
  final DeviceRepository deviceRepository;

  SwitchModulesController({
    required this.clientId,
    required this.clientRepository,
    required this.deviceRepository,
  }) : super(const AsyncValue.loading()) {
    load();
  }

  Future<void> load() async {
    state = const AsyncValue.loading();
    try {
      final modules = await clientRepository.listModules(clientId);
      state = AsyncValue.data(modules);
    } catch (error, stack) {
      state = AsyncValue.error(error, stack);
    }
  }

  Future<void> sendCommand({
    required String serverId,
    required String compId,
    required int mode,
    required int status,
    required int value,
  }) async {
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

    final modules = state.value ?? [];
    final exists = modules.any((module) => module.compId == compId);
    final updated = modules
        .map(
          (module) => module.compId == compId
              ? SwitchModule(
                  clientId: module.clientId,
                  compId: module.compId,
                  mode: mode,
                  status: status,
                  value: value,
                )
              : module,
        )
        .toList();

    if (!exists) {
      updated.add(
        SwitchModule(
          clientId: clientId,
          compId: compId,
          mode: mode,
          status: status,
          value: value,
        ),
      );
    }

    state = AsyncValue.data(updated);
  }

  String _normalizeId(String devId) {
    if (devId.startsWith('RSW-')) {
      return devId;
    }
    return 'RSW-$devId';
  }
}
