import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import '../../state/providers.dart';
import 'switch_screen.dart';

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

  Future<void> _bind(String clientId) async {
    await ref
        .read(deviceRepositoryProvider)
        .bindSlave(serverId: widget.serverId, clientId: clientId);
    await _loadSeen();
  }

  Future<void> _unbind(String clientId) async {
    await ref
        .read(deviceRepositoryProvider)
        .unbindSlave(serverId: widget.serverId, clientId: clientId);
  }

  Future<void> _requestStatus(String clientId) async {
    await ref
        .read(deviceRepositoryProvider)
        .requestStatus(serverId: widget.serverId, devId: clientId);
  }

  @override
  Widget build(BuildContext context) {
    final boundAsync = ref.watch(clientsProvider(widget.serverId));
    final statusAsync = ref.watch(gatewayStatusProvider(widget.serverId));

    return Scaffold(
      appBar: AppBar(
        title: Text('Gateway ${_displayId(widget.serverId)}'),
        actions: [
          IconButton(onPressed: _loadSeen, icon: const Icon(Icons.refresh)),
        ],
      ),
      body: RefreshIndicator(
        onRefresh: _loadSeen,
        child: ListView(
          padding: const EdgeInsets.all(20),
          children: [
            _HeaderCard(serverId: widget.serverId, statusAsync: statusAsync),
            const SizedBox(height: 18),
            const _SectionTitle('Connected devices'),
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
                          title: 'Slave ${_displayId(client.clientId)}',
                          subtitle: 'IP ${client.ip}',
                          statusLabel: 'Connected',
                          onPrimary: () => Navigator.push(
                            context,
                            MaterialPageRoute(
                              builder: (_) => SwitchScreen(
                                serverId: widget.serverId,
                                clientId: client.clientId,
                              ),
                            ),
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
              error: (_, __) => const _EmptySection(
                message: 'Failed to load connected devices.',
              ),
            ),
            const SizedBox(height: 18),
            const _SectionTitle('Discovered devices'),
            const SizedBox(height: 8),
            if (_loading)
              const Center(child: CircularProgressIndicator())
            else if (_error != null)
              _EmptySection(message: _error ?? 'Discovery failed.')
            else
              boundAsync.when(
                data: (clients) {
                  final boundIds = clients.map((c) => c.clientId).toSet();
                  final unbound = _seen.where((entry) {
                    final id = entry['clientID']?.toString() ?? '';
                    return id.isNotEmpty && !boundIds.contains(id);
                  }).toList();
                  if (unbound.isEmpty) {
                    return const _EmptySection(
                      message: 'No unbound devices discovered yet.',
                    );
                  }
                  return Column(
                    children: unbound
                        .map(
                          (entry) => _DeviceCard(
                            title:
                                'Slave ${_displayId(entry['clientID']?.toString() ?? '')}',
                            subtitle: 'IP ${entry['ip'] ?? '-'}',
                            statusLabel: 'New',
                            onPrimary: () =>
                                _bind(entry['clientID']?.toString() ?? ''),
                            primaryLabel: 'Bind',
                          ),
                        )
                        .toList(),
                  );
                },
                loading: () => const Center(child: CircularProgressIndicator()),
                error: (_, __) => const _EmptySection(
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
              'Unbind Slave ${_displayId(clientId)}?',
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
  final AsyncValue<bool> statusAsync;

  const _HeaderCard({required this.serverId, required this.statusAsync});

  @override
  Widget build(BuildContext context) {
    final label = statusAsync.when(
      data: (online) => online ? 'Gateway online' : 'Gateway offline',
      loading: () => 'Checking gateway...',
      error: (_, __) => 'Gateway status unknown',
    );
    final color = statusAsync.when(
      data: (online) =>
          online ? const Color(0xFF1E9E7A) : const Color(0xFFE27D60),
      loading: () => const Color(0xFF7A8C8B),
      error: (_, __) => const Color(0xFF7A8C8B),
    );

    return Container(
      padding: const EdgeInsets.all(16),
      decoration: BoxDecoration(
        color: Colors.white,
        borderRadius: BorderRadius.circular(18),
        border: Border.all(color: const Color(0xFFE5ECEB)),
      ),
      child: Row(
        children: [
          Container(
            width: 44,
            height: 44,
            decoration: BoxDecoration(
              color: color.withOpacity(0.12),
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
                  'Gateway ${_displayId(serverId)}',
                  style: const TextStyle(fontWeight: FontWeight.w700),
                ),
                const SizedBox(height: 4),
                Text(
                  label,
                  style: TextStyle(color: Colors.black.withOpacity(0.6)),
                ),
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
      decoration: BoxDecoration(
        color: Colors.white,
        borderRadius: BorderRadius.circular(14),
        border: Border.all(color: const Color(0xFFE5ECEB)),
      ),
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
    return Container(
      margin: const EdgeInsets.only(bottom: 12),
      padding: const EdgeInsets.all(16),
      decoration: BoxDecoration(
        color: Colors.white,
        borderRadius: BorderRadius.circular(16),
        border: Border.all(color: const Color(0xFFE5ECEB)),
      ),
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
          Text(
            subtitle,
            style: TextStyle(color: Colors.black.withOpacity(0.6)),
          ),
          const SizedBox(height: 10),
          Row(
            children: [
              Container(
                padding: const EdgeInsets.symmetric(
                  horizontal: 10,
                  vertical: 4,
                ),
                decoration: BoxDecoration(
                  color: const Color(0xFF0F7B7A).withOpacity(0.12),
                  borderRadius: BorderRadius.circular(999),
                ),
                child: Text(
                  statusLabel,
                  style: const TextStyle(
                    fontWeight: FontWeight.w600,
                    color: Color(0xFF0F7B7A),
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

String _displayId(String value) {
  if (value.startsWith('RSW-')) {
    return value.substring(4);
  }
  return value;
}
