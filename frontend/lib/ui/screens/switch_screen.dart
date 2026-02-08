import 'package:flutter/material.dart';
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
    final controller = ref.read(
      switchModulesProvider(widget.clientId).notifier,
    );

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
      body: Container(
        decoration: const BoxDecoration(
          gradient: LinearGradient(
            colors: [Color(0xFFF7F4EE), Color(0xFFE6F1F0)],
            begin: Alignment.topLeft,
            end: Alignment.bottomRight,
          ),
        ),
        child: modulesAsync.when(
          data: (modules) {
            if (modules.isEmpty) {
              return const Center(child: Text('No modules available.'));
            }
            return ListView.separated(
              padding: const EdgeInsets.all(18),
              itemCount: modules.length,
              separatorBuilder: (_, __) => const SizedBox(height: 12),
              itemBuilder: (_, index) {
                final module = modules[index];
                return _SwitchCard(
                  module: module,
                  pending: controller.isPending(module.compId),
                  onModeChanged: (mode) => _sendUpdate(module, mode: mode),
                  onStatusChanged: (status) =>
                      _sendUpdate(module, status: status),
                  onValueChanged: (value) => _sendUpdate(module, value: value),
                );
              },
            );
          },
          loading: () => const Center(child: CircularProgressIndicator()),
          error: (_, __) =>
              const Center(child: Text('Unable to load modules.')),
        ),
      ),
    );
  }

  Future<void> _sendUpdate(
    SwitchModule module, {
    int? mode,
    int? status,
    int? value,
  }) async {
    try {
      await ref
          .read(switchModulesProvider(widget.clientId).notifier)
          .sendCommand(
            serverId: widget.serverId,
            compId: module.compId,
            mode: mode ?? module.mode,
            status: status ?? module.status,
            value: value ?? module.value,
          );
    } catch (_) {
      if (!mounted) {
        return;
      }
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('Device not bound or offline.')),
      );
    }
  }
}

class _SwitchCard extends StatelessWidget {
  final SwitchModule module;
  final bool pending;
  final ValueChanged<int> onModeChanged;
  final ValueChanged<int> onStatusChanged;
  final ValueChanged<int> onValueChanged;

  const _SwitchCard({
    required this.module,
    required this.pending,
    required this.onModeChanged,
    required this.onStatusChanged,
    required this.onValueChanged,
  });

  @override
  Widget build(BuildContext context) {
    final isAuto = module.mode == 1;
    final isOn = module.status == 1;
    final accent = isOn ? const Color(0xFF0F7B7A) : const Color(0xFF8E9A9A);

    return Container(
      padding: const EdgeInsets.all(16),
      decoration: BoxDecoration(
        color: Colors.white,
        borderRadius: BorderRadius.circular(18),
        border: Border.all(color: const Color(0xFFE5ECEB)),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceBetween,
            children: [
              Text(
                module.compId,
                style: const TextStyle(
                  fontSize: 18,
                  fontWeight: FontWeight.w700,
                ),
              ),
              if (pending)
                const SizedBox(
                  height: 18,
                  width: 18,
                  child: CircularProgressIndicator(strokeWidth: 2),
                ),
            ],
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
                  onSelectionChanged: pending
                      ? null
                      : (values) {
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
                    onChanged: pending
                        ? null
                        : (value) => onStatusChanged(value ? 1 : 0),
                  ),
                ],
              ),
            ],
          ),
          const SizedBox(height: 12),
          Text(
            'Level ${module.value}',
            style: TextStyle(fontWeight: FontWeight.w600, color: accent),
          ),
          Slider(
            min: 0,
            max: 1000,
            value: module.value.toDouble().clamp(0, 1000),
            onChanged: pending
                ? null
                : (value) => onValueChanged(value.round()),
          ),
        ],
      ),
    );
  }
}
