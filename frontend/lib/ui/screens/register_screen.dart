import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import '../../state/providers.dart';
import 'config_screen.dart';
import 'network_devices_screen.dart';

class RegisterScreen extends ConsumerWidget {
  static const routeName = '/register-device';

  const RegisterScreen({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final serversAsync = ref.watch(serversProvider);

    return Scaffold(
      appBar: AppBar(
        title: const Text('Gateway Discovery'),
        actions: [
          IconButton(
            onPressed: () => ref.refresh(serversProvider),
            icon: const Icon(Icons.refresh),
          ),
        ],
      ),
      body: Padding(
        padding: const EdgeInsets.all(20),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            const Text(
              'Gateways show up automatically when they are online.',
              style: TextStyle(fontSize: 16, fontWeight: FontWeight.w600),
            ),
            const SizedBox(height: 6),
            Text(
              'Use a gateway to discover devices or configure WiFi.',
              style: TextStyle(color: Colors.black.withOpacity(0.6)),
            ),
            const SizedBox(height: 20),
            Expanded(
              child: serversAsync.when(
                data: (servers) {
                  if (servers.isEmpty) {
                    return const Center(child: Text('No gateways online yet.'));
                  }
                  return ListView.separated(
                    itemCount: servers.length,
                    separatorBuilder: (_, __) => const SizedBox(height: 12),
                    itemBuilder: (_, index) {
                      final server = servers[index];
                      return _GatewayTile(
                        serverId: server.serverId,
                        ip: server.ip,
                        onConfigure: () => Navigator.push(
                          context,
                          MaterialPageRoute(
                            builder: (_) =>
                                ConfigScreen(deviceId: server.serverId),
                          ),
                        ),
                        onDiscover: () => Navigator.push(
                          context,
                          MaterialPageRoute(
                            builder: (_) =>
                                NetworkDevicesScreen(serverId: server.serverId),
                          ),
                        ),
                      );
                    },
                  );
                },
                loading: () => const Center(child: CircularProgressIndicator()),
                error: (_, __) =>
                    const Center(child: Text('Unable to load gateways.')),
              ),
            ),
          ],
        ),
      ),
    );
  }
}

class _GatewayTile extends ConsumerWidget {
  final String serverId;
  final String ip;
  final VoidCallback onConfigure;
  final VoidCallback onDiscover;

  const _GatewayTile({
    required this.serverId,
    required this.ip,
    required this.onConfigure,
    required this.onDiscover,
  });

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final statusAsync = ref.watch(gatewayStatusProvider(serverId));
    final statusLabel = statusAsync.when(
      data: (online) => online ? 'Online' : 'Offline',
      loading: () => 'Checking',
      error: (_, __) => 'Unknown',
    );
    final statusColor = statusAsync.when(
      data: (online) =>
          online ? const Color(0xFF1E9E7A) : const Color(0xFFE27D60),
      loading: () => const Color(0xFF7A8C8B),
      error: (_, __) => const Color(0xFF7A8C8B),
    );

    return Container(
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
              Text(
                'Gateway ${_displayId(serverId)}',
                style: const TextStyle(fontWeight: FontWeight.w700),
              ),
              const Spacer(),
              Container(
                padding: const EdgeInsets.symmetric(
                  horizontal: 10,
                  vertical: 4,
                ),
                decoration: BoxDecoration(
                  color: statusColor.withOpacity(0.12),
                  borderRadius: BorderRadius.circular(999),
                ),
                child: Text(
                  statusLabel,
                  style: TextStyle(
                    fontWeight: FontWeight.w600,
                    color: statusColor,
                  ),
                ),
              ),
            ],
          ),
          const SizedBox(height: 6),
          Text(
            'IP $ip',
            style: TextStyle(color: Colors.black.withOpacity(0.6)),
          ),
          const SizedBox(height: 12),
          Wrap(
            spacing: 8,
            children: [
              FilledButton(
                onPressed: onDiscover,
                child: const Text('Discover'),
              ),
              OutlinedButton(
                onPressed: onConfigure,
                child: const Text('Configure'),
              ),
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
