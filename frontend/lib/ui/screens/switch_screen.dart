import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import '../../models/switch_module.dart';
import '../../state/providers.dart';

class SwitchScreen extends ConsumerStatefulWidget {
  final String serverId;
  final String clientId;

  const SwitchScreen({
    super.key,
    required this.serverId,
    required this.clientId,
  });

  @override
  ConsumerState<SwitchScreen> createState() => _SwitchScreenState();
}

class _SwitchScreenState extends ConsumerState<SwitchScreen> {
  @override
  void initState() {
    super.initState();
    final token = ref.read(authControllerProvider).token;
    ref.read(socketClientProvider).connect(token: token);
  }

  @override
  Widget build(BuildContext context) {
    ref.listen(socketStatusProvider, (_, next) {
      next.whenData((data) {
        ref
            .read(switchModulesProvider(widget.clientId).notifier)
            .applySocketUpdate(data);
      });
    });
    ref.listen(socketResponseProvider, (_, next) {
      next.whenData((data) {
        ref
            .read(switchModulesProvider(widget.clientId).notifier)
            .applySocketUpdate(data);
      });
    });

    final modulesAsync = ref.watch(switchModulesProvider(widget.clientId));

    return Scaffold(
      appBar: AppBar(
        title: Text('Switches ${widget.clientId}'),
        actions: [
          IconButton(
            icon: const Icon(Icons.refresh),
            onPressed: () async {
              await ref
                  .read(deviceRepositoryProvider)
                  .requestStatus(
                    serverId: widget.serverId,
                    devId: widget.clientId,
                  );
            },
          ),
        ],
      ),
      body: modulesAsync.when(
        data: (modules) {
          if (modules.isEmpty) {
            return const Center(child: Text('No modules available.'));
          }
          return ListView.separated(
            padding: const EdgeInsets.all(16),
            itemCount: modules.length,
            separatorBuilder: (_, __) => const SizedBox(height: 12),
            itemBuilder: (_, index) {
              final module = modules[index];
              return _SwitchCard(
                module: module,
                onModeChanged: (mode) => _sendUpdate(module, mode: mode),
                onStatusChanged: (status) =>
                    _sendUpdate(module, status: status),
                onValueChanged: (value) => _sendUpdate(module, value: value),
              );
            },
          );
        },
        loading: () => const Center(child: CircularProgressIndicator()),
        error: (_, __) => const Center(child: Text('Unable to load modules.')),
      ),
    );
  }

  Future<void> _sendUpdate(
    SwitchModule module, {
    int? mode,
    int? status,
    int? value,
  }) async {
    await ref
        .read(switchModulesProvider(widget.clientId).notifier)
        .sendCommand(
          serverId: widget.serverId,
          compId: module.compId,
          mode: mode ?? module.mode,
          status: status ?? module.status,
          value: value ?? module.value,
        );
  }
}

class _SwitchCard extends StatelessWidget {
  final SwitchModule module;
  final ValueChanged<int> onModeChanged;
  final ValueChanged<int> onStatusChanged;
  final ValueChanged<int> onValueChanged;

  const _SwitchCard({
    required this.module,
    required this.onModeChanged,
    required this.onStatusChanged,
    required this.onValueChanged,
  });

  @override
  Widget build(BuildContext context) {
    final isAuto = module.mode == 1;
    final isOn = module.status == 1;

    return Container(
      padding: const EdgeInsets.all(16),
      decoration: BoxDecoration(
        color: Colors.white,
        borderRadius: BorderRadius.circular(16),
        boxShadow: [
          BoxShadow(
            color: Colors.black.withOpacity(0.05),
            blurRadius: 16,
            offset: const Offset(0, 8),
          ),
        ],
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(
            module.compId,
            style: const TextStyle(fontSize: 18, fontWeight: FontWeight.w600),
          ),
          const SizedBox(height: 12),
          Row(
            children: [
              Expanded(
                child: SegmentedButton<int>(
                  segments: const [
                    ButtonSegment(value: 0, label: Text('Manual')),
                    ButtonSegment(value: 1, label: Text('Auto')),
                  ],
                  selected: {isAuto ? 1 : 0},
                  onSelectionChanged: (values) {
                    onModeChanged(values.first);
                  },
                ),
              ),
              const SizedBox(width: 12),
              Column(
                children: [
                  const Text('Power', style: TextStyle(fontSize: 12)),
                  Switch(
                    value: isOn,
                    onChanged: (value) => onStatusChanged(value ? 1 : 0),
                  ),
                ],
              ),
            ],
          ),
          const SizedBox(height: 12),
          Text('Level ${module.value}'),
          Slider(
            min: 0,
            max: 1000,
            value: module.value.toDouble().clamp(0, 1000),
            onChanged: (value) => onValueChanged(value.round()),
          ),
        ],
      ),
    );
  }
}

String _displayId(String value) {
  if (value.startsWith('RSW-')) {
    return value.substring(4);
  }
  return value;
}
