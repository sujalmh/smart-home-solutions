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
    return Scaffold(
      appBar: AppBar(
        title: Text('Network ${_displayId(widget.serverId)}'),
        actions: [
          IconButton(onPressed: _loadSeen, icon: const Icon(Icons.refresh)),
        ],
      ),
      body: ListView(
        padding: const EdgeInsets.all(16),
        children: [
          _buildSectionTitle('Bound Devices'),
          boundAsync.when(
            data: (clients) {
              if (clients.isEmpty) {
                return const Padding(
                  padding: EdgeInsets.symmetric(vertical: 8),
                  child: Text('No bound devices yet.'),
                );
              }
              return Column(
                children: clients
                    .map(
                      (client) => _DeviceTile(
                        title: 'Slave ${_displayId(client.clientId)}',
                        subtitle: 'IP ${client.ip}',
                        onPrimary: () => Navigator.push(
                          context,
                          MaterialPageRoute(
                            builder: (_) => SwitchScreen(
                              serverId: widget.serverId,
                              clientId: client.clientId,
                            ),
                          ),
                        ),
                        primaryLabel: 'View',
                        onSecondary: () => _requestStatus(client.clientId),
                        secondaryLabel: 'Status',
                        onTertiary: () => _unbind(client.clientId),
                        tertiaryLabel: 'Unbind',
                      ),
                    )
                    .toList(),
              );
            },
            loading: () => const Padding(
              padding: EdgeInsets.symmetric(vertical: 8),
              child: Center(child: CircularProgressIndicator()),
            ),
            error: (_, __) => const Padding(
              padding: EdgeInsets.symmetric(vertical: 8),
              child: Text('Failed to load bound devices.'),
            ),
          ),
          const SizedBox(height: 24),
          _buildSectionTitle('Discovered Devices'),
          if (_loading)
            const Padding(
              padding: EdgeInsets.symmetric(vertical: 8),
              child: Center(child: CircularProgressIndicator()),
            )
          else if (_error != null)
            Padding(
              padding: const EdgeInsets.symmetric(vertical: 8),
              child: Text(_error ?? 'Failed to load discovered devices.'),
            )
          else
            boundAsync.when(
              data: (clients) {
                final boundIds = clients.map((c) => c.clientId).toSet();
                final unbound = _seen.where((entry) {
                  final id = entry['clientID']?.toString() ?? '';
                  return id.isNotEmpty && !boundIds.contains(id);
                }).toList();
                if (unbound.isEmpty) {
                  return const Padding(
                    padding: EdgeInsets.symmetric(vertical: 8),
                    child: Text('No unbound devices discovered yet.'),
                  );
                }
                return Column(
                  children: unbound
                      .map(
                        (entry) => _DeviceTile(
                          title:
                              'Slave ${_displayId(entry['clientID']?.toString() ?? '')}',
                          subtitle: 'IP ${entry['ip'] ?? '-'}',
                          onPrimary: () =>
                              _bind(entry['clientID']?.toString() ?? ''),
                          primaryLabel: 'Bind',
                        ),
                      )
                      .toList(),
                );
              },
              loading: () => const Padding(
                padding: EdgeInsets.symmetric(vertical: 8),
                child: Center(child: CircularProgressIndicator()),
              ),
              error: (_, __) => const Padding(
                padding: EdgeInsets.symmetric(vertical: 8),
                child: Text('Failed to load bound devices.'),
              ),
            ),
        ],
      ),
    );
  }

  Widget _buildSectionTitle(String text) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 8),
      child: Text(
        text,
        style: const TextStyle(fontSize: 16, fontWeight: FontWeight.w600),
      ),
    );
  }
}

class _DeviceTile extends StatelessWidget {
  final String title;
  final String subtitle;
  final VoidCallback onPrimary;
  final String primaryLabel;
  final VoidCallback? onSecondary;
  final String? secondaryLabel;
  final VoidCallback? onTertiary;
  final String? tertiaryLabel;

  const _DeviceTile({
    required this.title,
    required this.subtitle,
    required this.onPrimary,
    required this.primaryLabel,
    this.onSecondary,
    this.secondaryLabel,
    this.onTertiary,
    this.tertiaryLabel,
  });

  @override
  Widget build(BuildContext context) {
    return Card(
      margin: const EdgeInsets.only(bottom: 12),
      child: Padding(
        padding: const EdgeInsets.all(12),
        child: Row(
          children: [
            Expanded(
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text(
                    title,
                    style: const TextStyle(fontWeight: FontWeight.w600),
                  ),
                  const SizedBox(height: 4),
                  Text(subtitle),
                ],
              ),
            ),
            TextButton(onPressed: onPrimary, child: Text(primaryLabel)),
            if (onSecondary != null && secondaryLabel != null) ...[
              const SizedBox(width: 4),
              TextButton(onPressed: onSecondary, child: Text(secondaryLabel!)),
            ],
            if (onTertiary != null && tertiaryLabel != null) ...[
              const SizedBox(width: 4),
              OutlinedButton(
                onPressed: onTertiary,
                child: Text(tertiaryLabel!),
              ),
            ],
          ],
        ),
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
