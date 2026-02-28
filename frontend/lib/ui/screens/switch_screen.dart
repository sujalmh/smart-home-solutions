import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import '../../data/socket_client.dart';
import '../../models/switch_module.dart';
import '../../state/providers.dart';
import '../../state/switch_modules_controller.dart';

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
    Future.microtask(() {
      ref
          .read(switchModulesProvider(widget.clientId).notifier)
          .refreshFromDevice(widget.serverId, silent: true);
    });
  }

  @override
  Widget build(BuildContext context) {
    ref.listen(socketConnectionProvider, (_, next) {
      next.whenData((connection) {
        if (connection == SocketConnectionState.connected) {
          ref
              .read(switchModulesProvider(widget.clientId).notifier)
              .refreshFromDevice(widget.serverId, silent: true);
        }
      });
    });

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
              try {
                await ref
                    .read(switchModulesProvider(widget.clientId).notifier)
                    .refreshFromDevice(widget.serverId, refresh: true);
              } catch (_) {
                if (!context.mounted) {
                  return;
                }
                ScaffoldMessenger.of(context).showSnackBar(
                  const SnackBar(content: Text('Unable to refresh status.')),
                );
              }
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
              separatorBuilder: (_, _) => const SizedBox(height: 12),
              itemBuilder: (_, index) {
                final module = modules[index];
                return _SwitchCard(
                  module: module,
                  pending: controller.isPending(module.compId),
                  stale: controller.isStale(module.compId),
                  rateLimited: controller.isRateLimited(module.compId),
                  onModeChanged: (mode) => _sendUpdate(module, mode: mode),
                  onStatusChanged: (status) =>
                      _sendUpdate(module, status: status),
                  onValueCommitted: (value) =>
                      _sendUpdate(module, value: value),
                );
              },
            );
          },
          loading: () => const Center(child: CircularProgressIndicator()),
          error: (_, _) => const Center(child: Text('Unable to load modules.')),
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
    } on CommandRateLimitException catch (error) {
      if (!mounted) {
        return;
      }
      final waitSeconds = (error.waitMs / 1000).ceil();
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text('Please wait $waitSeconds s before next input.'),
        ),
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

class _SwitchCard extends StatefulWidget {
  final SwitchModule module;
  final bool pending;
  final bool stale;
  final bool rateLimited;
  final ValueChanged<int> onModeChanged;
  final ValueChanged<int> onStatusChanged;
  final ValueChanged<int> onValueCommitted;

  const _SwitchCard({
    required this.module,
    required this.pending,
    required this.stale,
    required this.rateLimited,
    required this.onModeChanged,
    required this.onStatusChanged,
    required this.onValueCommitted,
  });

  @override
  State<_SwitchCard> createState() => _SwitchCardState();
}

class _SwitchCardState extends State<_SwitchCard> {
  late double _dragValue;
  bool _isDragging = false;

  @override
  void initState() {
    super.initState();
    _dragValue = widget.module.value.toDouble().clamp(0, 1000);
  }

  @override
  void didUpdateWidget(covariant _SwitchCard oldWidget) {
    super.didUpdateWidget(oldWidget);
    if (!_isDragging && oldWidget.module.value != widget.module.value) {
      _dragValue = widget.module.value.toDouble().clamp(0, 1000);
    }
  }

  @override
  Widget build(BuildContext context) {
    final module = widget.module;
    final pending = widget.pending;
    final stale = widget.stale;
    final rateLimited = widget.rateLimited;
    final blocked = pending || rateLimited;
    final isAuto = module.mode == 1;
    final isOn = module.status == 1;
    final accent = isOn ? const Color(0xFF0F7B7A) : const Color(0xFF8E9A9A);
    final levelLabel = _isDragging ? _dragValue.round() : module.value;

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
                )
              else if (rateLimited)
                Container(
                  padding: const EdgeInsets.symmetric(
                    horizontal: 8,
                    vertical: 4,
                  ),
                  decoration: BoxDecoration(
                    color: const Color(0xFF9AB0D7).withValues(alpha: 0.2),
                    borderRadius: BorderRadius.circular(999),
                  ),
                  child: const Text(
                    'Input limit',
                    style: TextStyle(fontSize: 12, fontWeight: FontWeight.w600),
                  ),
                )
              else if (stale)
                Container(
                  padding: const EdgeInsets.symmetric(
                    horizontal: 8,
                    vertical: 4,
                  ),
                  decoration: BoxDecoration(
                    color: const Color(0xFFF2C26A).withValues(alpha: 0.22),
                    borderRadius: BorderRadius.circular(999),
                  ),
                  child: const Text(
                    'Sync delayed',
                    style: TextStyle(fontSize: 12, fontWeight: FontWeight.w600),
                  ),
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
                  onSelectionChanged: blocked
                      ? null
                      : (values) {
                          widget.onModeChanged(values.first);
                        },
                ),
              ),
              const SizedBox(width: 12),
              Column(
                children: [
                  const Text('Power', style: TextStyle(fontSize: 12)),
                  Switch(
                    value: isOn,
                    onChanged: blocked
                        ? null
                        : (value) => widget.onStatusChanged(value ? 1 : 0),
                  ),
                ],
              ),
            ],
          ),
          const SizedBox(height: 12),
          Text(
            'Level $levelLabel',
            style: TextStyle(fontWeight: FontWeight.w600, color: accent),
          ),
          Slider(
            min: 0,
            max: 1000,
            value: _dragValue,
            onChanged: blocked
                ? null
                : (value) {
                    setState(() {
                      _isDragging = true;
                      _dragValue = value;
                    });
                  },
            onChangeEnd: blocked
                ? null
                : (value) {
                    setState(() {
                      _isDragging = false;
                      _dragValue = value;
                    });
                    widget.onValueCommitted(value.round());
                  },
          ),
        ],
      ),
    );
  }
}
