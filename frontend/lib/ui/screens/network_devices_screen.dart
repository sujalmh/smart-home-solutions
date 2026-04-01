import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:go_router/go_router.dart';

import '../../config/app_colors.dart';
import '../../config/app_decorations.dart';
import '../../state/providers.dart';
import '../../utils/id_utils.dart';

class NetworkDevicesScreen extends ConsumerStatefulWidget {
  final String serverId;

  const NetworkDevicesScreen({super.key, required this.serverId});

  @override
  ConsumerState<NetworkDevicesScreen> createState() =>
      _NetworkDevicesScreenState();
}

class _NetworkDevicesScreenState extends ConsumerState<NetworkDevicesScreen> {
  bool _loading = false;
  String? _error;
  List<Map<String, dynamic>> _seen = [];

  @override
  void initState() {
    super.initState();
    _loadSeen();
    // Fetch gateway online/offline status for this server on first load.
    Future.microtask(
      () => ref
          .read(gatewayStatusNotifierProvider.notifier)
          .refresh(widget.serverId),
    );
  }

  Future<void> _loadSeen() async {
    setState(() {
      _loading = true;
      _error = null;
    });
    try {
      final seen = await ref
          .read(deviceRepositoryProvider)
          .listSeenSlaves(serverId: widget.serverId);
      setState(() => _seen = seen);
    } catch (error) {
      setState(() => _error = error.toString());
    } finally {
      setState(() => _loading = false);
    }
  }

  Future<void> _bind(String clientId, {int channelCount = 3}) async {
    try {
      await ref
          .read(deviceRepositoryProvider)
          .bindSlave(
            serverId: widget.serverId,
            clientId: clientId,
            channelCount: channelCount,
          );
      ref.invalidate(clientsProvider(widget.serverId));
      await _loadSeen();
    } catch (e) {
      if (mounted) {
        ScaffoldMessenger.of(
          context,
        ).showSnackBar(SnackBar(content: Text('Bind failed: $e')));
      }
    }
  }

  void _showBindSheet(String clientId) {
    showModalBottomSheet<void>(
      context: context,
      showDragHandle: true,
      builder: (ctx) => Padding(
        padding: const EdgeInsets.fromLTRB(20, 4, 20, 32),
        child: Column(
          mainAxisSize: MainAxisSize.min,
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              'Bind Slave ${displayId(clientId)}',
              style: const TextStyle(fontSize: 18, fontWeight: FontWeight.w700),
            ),
            const SizedBox(height: 6),
            const Text('How many channels does this device have?'),
            const SizedBox(height: 16),
            Row(
              children: [
                Expanded(
                  child: OutlinedButton(
                    onPressed: () {
                      Navigator.pop(ctx);
                      _bind(clientId, channelCount: 1);
                    },
                    child: const Text('1-channel'),
                  ),
                ),
                const SizedBox(width: 12),
                Expanded(
                  child: FilledButton(
                    onPressed: () {
                      Navigator.pop(ctx);
                      _bind(clientId, channelCount: 3);
                    },
                    child: const Text('3-channel'),
                  ),
                ),
              ],
            ),
          ],
        ),
      ),
    );
  }

  Future<void> _unbind(String clientId) async {
    try {
      await ref
          .read(deviceRepositoryProvider)
          .unbindSlave(serverId: widget.serverId, clientId: clientId);
      ref.invalidate(clientsProvider(widget.serverId));
    } catch (e) {
      if (mounted) {
        ScaffoldMessenger.of(
          context,
        ).showSnackBar(SnackBar(content: Text('Unbind failed: $e')));
      }
    }
  }

  Future<void> _requestStatus(String clientId) async {
    try {
      await ref
          .read(deviceRepositoryProvider)
          .requestStatus(
            serverId: widget.serverId,
            devId: clientId,
            refresh: true,
          );
    } catch (e) {
      if (mounted) {
        ScaffoldMessenger.of(
          context,
        ).showSnackBar(SnackBar(content: Text('Status request failed: $e')));
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    final boundAsync = ref.watch(clientsProvider(widget.serverId));
    // bool? — null = not yet fetched ("Checking")
    final online = ref.watch(gatewayStatusProvider(widget.serverId));

    return Scaffold(
      appBar: AppBar(
        title: Text('Gateway ${displayId(widget.serverId)}'),
        actions: [
          IconButton(onPressed: _loadSeen, icon: const Icon(Icons.refresh)),
        ],
      ),
      body: RefreshIndicator(
        onRefresh: _loadSeen,
        child: ListView(
          padding: const EdgeInsets.all(20),
          children: [
            _HeaderCard(serverId: widget.serverId, online: online),
            const SizedBox(height: 18),
            const _SectionTitle('My devices'),
            const SizedBox(height: 8),
            boundAsync.when(
              data: (clients) {
                if (clients.isEmpty) {
                  return const _EmptySection(message: 'No devices bound yet.');
                }
                return Column(
                  children: clients
                      .map(
                        (client) => _DeviceCard(
                          title: 'Slave ${displayId(client.clientId)}',
                          subtitle: 'IP ${client.ip}',
                          statusLabel: 'Connected',
                          onPrimary: () => context.push(
                            '/switch/${widget.serverId}/${client.clientId}',
                            extra: {'moduleCount': client.moduleCount},
                          ),
                          primaryLabel: 'Control',
                          onSecondary: () => _requestStatus(client.clientId),
                          secondaryLabel: 'Refresh',
                          onOverflow: () => _showUnbindSheet(client.clientId),
                        ),
                      )
                      .toList(),
                );
              },
              loading: () => const Center(child: CircularProgressIndicator()),
              error: (_, _) => const _EmptySection(
                message: 'Failed to load connected devices.',
              ),
            ),
            const SizedBox(height: 18),
            const _SectionTitle('Available devices'),
            const SizedBox(height: 8),
            if (_loading)
              const Center(child: CircularProgressIndicator())
            else if (_error != null)
              _EmptySection(message: _error ?? 'Discovery failed.')
            else
              boundAsync.when(
                data: (clients) {
                  final boundIds = clients
                      .map((c) => wireId(c.clientId))
                      .toSet();
                  final unbound = _seen.where((entry) {
                    final id = wireId(entry['clientID']?.toString() ?? '');
                    return id.isNotEmpty && !boundIds.contains(id);
                  }).toList();
                  if (unbound.isEmpty) {
                    return const _EmptySection(
                      message: 'No available devices yet.',
                    );
                  }
                  return Column(
                    children: unbound
                        .map(
                          (entry) => _DeviceCard(
                            title:
                                'Slave ${displayId(entry['clientID']?.toString() ?? '')}',
                            subtitle: 'IP ${entry['ip'] ?? '-'}',
                            statusLabel: 'New',
                            onPrimary: () => _showBindSheet(
                              entry['clientID']?.toString() ?? '',
                            ),
                            primaryLabel: 'Bind',
                          ),
                        )
                        .toList(),
                  );
                },
                loading: () => const Center(child: CircularProgressIndicator()),
                error: (_, _) => const _EmptySection(
                  message: 'Failed to load discovered devices.',
                ),
              ),
          ],
        ),
      ),
    );
  }

  void _showUnbindSheet(String clientId) {
    showModalBottomSheet<void>(
      context: context,
      showDragHandle: true,
      builder: (context) => Padding(
        padding: const EdgeInsets.all(20),
        child: Column(
          mainAxisSize: MainAxisSize.min,
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              'Unbind Slave ${displayId(clientId)}?',
              style: const TextStyle(fontSize: 18, fontWeight: FontWeight.w700),
            ),
            const SizedBox(height: 8),
            const Text(
              'The device will stay visible in discovery but will be removed from your bound list.',
            ),
            const SizedBox(height: 16),
            Row(
              children: [
                Expanded(
                  child: OutlinedButton(
                    onPressed: () => Navigator.pop(context),
                    child: const Text('Cancel'),
                  ),
                ),
                const SizedBox(width: 12),
                Expanded(
                  child: FilledButton(
                    onPressed: () async {
                      Navigator.pop(context);
                      await _unbind(clientId);
                    },
                    child: const Text('Unbind'),
                  ),
                ),
              ],
            ),
          ],
        ),
      ),
    );
  }
}

class _HeaderCard extends StatelessWidget {
  final String serverId;
  final bool? online;

  const _HeaderCard({required this.serverId, required this.online});

  @override
  Widget build(BuildContext context) {
    final c = context.colors;
    final label = online == null
        ? 'Checking gateway...'
        : (online! ? 'Gateway online' : 'Gateway offline');
    final color = online == null ? c.neutral : (online! ? c.online : c.offline);

    return Container(
      padding: const EdgeInsets.all(16),
      decoration: AppDecorations.section(c),
      child: Row(
        children: [
          Container(
            width: 44,
            height: 44,
            decoration: BoxDecoration(
              color: color.withValues(alpha: 0.12),
              borderRadius: BorderRadius.circular(12),
            ),
            child: Icon(Icons.router_outlined, color: color),
          ),
          const SizedBox(width: 12),
          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(
                  'Gateway ${displayId(serverId)}',
                  style: const TextStyle(fontWeight: FontWeight.w700),
                ),
                const SizedBox(height: 4),
                Text(label, style: TextStyle(color: c.subtitle)),
              ],
            ),
          ),
        ],
      ),
    );
  }
}

class _SectionTitle extends StatelessWidget {
  final String text;

  const _SectionTitle(this.text);

  @override
  Widget build(BuildContext context) {
    return Text(
      text,
      style: const TextStyle(fontSize: 16, fontWeight: FontWeight.w700),
    );
  }
}

class _EmptySection extends StatelessWidget {
  final String message;

  const _EmptySection({required this.message});

  @override
  Widget build(BuildContext context) {
    return Container(
      padding: const EdgeInsets.all(16),
      decoration: AppDecorations.section(context.colors),
      child: Text(message),
    );
  }
}

class _DeviceCard extends StatelessWidget {
  final String title;
  final String subtitle;
  final String statusLabel;
  final VoidCallback onPrimary;
  final String primaryLabel;
  final VoidCallback? onSecondary;
  final String? secondaryLabel;
  final VoidCallback? onOverflow;

  const _DeviceCard({
    required this.title,
    required this.subtitle,
    required this.statusLabel,
    required this.onPrimary,
    required this.primaryLabel,
    this.onSecondary,
    this.secondaryLabel,
    this.onOverflow,
  });

  @override
  Widget build(BuildContext context) {
    final c = context.colors;
    return Container(
      margin: const EdgeInsets.only(bottom: 12),
      padding: const EdgeInsets.all(16),
      decoration: AppDecorations.section(c),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Expanded(
                child: Text(
                  title,
                  style: const TextStyle(fontWeight: FontWeight.w700),
                ),
              ),
              if (onOverflow != null)
                IconButton(
                  onPressed: onOverflow,
                  icon: const Icon(Icons.more_vert),
                ),
            ],
          ),
          Text(subtitle, style: TextStyle(color: c.subtitle)),
          const SizedBox(height: 10),
          Row(
            children: [
              Container(
                padding: const EdgeInsets.symmetric(
                  horizontal: 10,
                  vertical: 4,
                ),
                decoration: AppDecorations.statusChip(c.primary),
                child: Text(
                  statusLabel,
                  style: TextStyle(
                    fontWeight: FontWeight.w600,
                    color: c.primary,
                  ),
                ),
              ),
              const Spacer(),
              TextButton(onPressed: onPrimary, child: Text(primaryLabel)),
              if (onSecondary != null && secondaryLabel != null) ...[
                const SizedBox(width: 6),
                OutlinedButton(
                  onPressed: onSecondary,
                  child: Text(secondaryLabel!),
                ),
              ],
            ],
          ),
        ],
      ),
    );
  }
}
