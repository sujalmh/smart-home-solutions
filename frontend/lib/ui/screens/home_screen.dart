import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import '../../models/server.dart';
import '../../state/providers.dart';
import 'config_screen.dart';
import 'network_devices_screen.dart';
import 'remote_login_screen.dart';
import 'room_switches_screen.dart';

class HomeScreen extends ConsumerWidget {
  const HomeScreen({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final serversAsync = ref.watch(serversProvider);
    return Scaffold(
      body: Container(
        decoration: const BoxDecoration(
          gradient: LinearGradient(
            colors: [Color(0xFFF7F4EE), Color(0xFFE6F1F0)],
            begin: Alignment.topLeft,
            end: Alignment.bottomRight,
          ),
        ),
        child: SafeArea(
          child: Padding(
            padding: const EdgeInsets.fromLTRB(20, 16, 20, 12),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                _Header(
                  onLogin: () =>
                      Navigator.pushNamed(context, RemoteLoginScreen.routeName),
                  onRefresh: () => ref.refresh(serversProvider),
                ),
                const SizedBox(height: 16),
                const Text(
                  'Gateways on your network',
                  style: TextStyle(
                    fontSize: 18,
                    fontWeight: FontWeight.w700,
                    color: Color(0xFF101414),
                  ),
                ),
                const SizedBox(height: 6),
                Text(
                  'Devices appear automatically when the master gateway is online.',
                  style: TextStyle(color: Colors.black.withOpacity(0.6)),
                ),
                const SizedBox(height: 18),
                Expanded(
                  child: serversAsync.when(
                    data: (servers) {
                      if (servers.isEmpty) {
                        return _EmptyState(
                          onRefresh: () => ref.refresh(serversProvider),
                        );
                      }
                      return ListView.separated(
                        itemCount: servers.length,
                        separatorBuilder: (_, __) => const SizedBox(height: 14),
                        itemBuilder: (_, index) => _GatewayCard(
                          server: servers[index],
                          onConfigure: () => Navigator.push(
                            context,
                            MaterialPageRoute(
                              builder: (_) => ConfigScreen(
                                deviceId: servers[index].serverId,
                              ),
                            ),
                          ),
                          onDevices: () => Navigator.push(
                            context,
                            MaterialPageRoute(
                              builder: (_) => NetworkDevicesScreen(
                                serverId: servers[index].serverId,
                              ),
                            ),
                          ),
                          onSwitches: () => Navigator.push(
                            context,
                            MaterialPageRoute(
                              builder: (_) => RoomSwitchesScreen(
                                serverId: servers[index].serverId,
                              ),
                            ),
                          ),
                        ),
                      );
                    },
                    loading: () =>
                        const Center(child: CircularProgressIndicator()),
                    error: (_, __) =>
                        const Center(child: Text('Unable to load gateways.')),
                  ),
                ),
              ],
            ),
          ),
        ),
      ),
    );
  }
}

class _Header extends StatelessWidget {
  final VoidCallback onLogin;
  final VoidCallback onRefresh;

  const _Header({required this.onLogin, required this.onRefresh});

  @override
  Widget build(BuildContext context) {
    return Row(
      children: [
        Expanded(
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: const [
              Text(
                'Smart Home',
                style: TextStyle(
                  fontSize: 30,
                  fontWeight: FontWeight.w700,
                  letterSpacing: -0.6,
                ),
              ),
              SizedBox(height: 6),
              Text(
                'Live control with real-time updates.',
                style: TextStyle(fontSize: 15, color: Color(0xFF5C6B6B)),
              ),
            ],
          ),
        ),
        IconButton(
          onPressed: onRefresh,
          icon: const Icon(Icons.refresh),
          tooltip: 'Refresh',
        ),
        IconButton(
          onPressed: onLogin,
          icon: const Icon(Icons.person_outline),
          tooltip: 'Account',
        ),
      ],
    );
  }
}

class _EmptyState extends StatelessWidget {
  final VoidCallback onRefresh;

  const _EmptyState({required this.onRefresh});

  @override
  Widget build(BuildContext context) {
    return Center(
      child: Container(
        padding: const EdgeInsets.all(20),
        decoration: BoxDecoration(
          color: Colors.white,
          borderRadius: BorderRadius.circular(20),
          boxShadow: [
            BoxShadow(
              color: Colors.black.withOpacity(0.06),
              blurRadius: 18,
              offset: const Offset(0, 10),
            ),
          ],
        ),
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            const Icon(
              Icons.router_outlined,
              size: 42,
              color: Color(0xFF0F7B7A),
            ),
            const SizedBox(height: 12),
            const Text(
              'No gateways found yet',
              style: TextStyle(fontSize: 18, fontWeight: FontWeight.w700),
            ),
            const SizedBox(height: 6),
            const Text(
              'Power on the master gateway and ensure it is online.',
              textAlign: TextAlign.center,
            ),
            const SizedBox(height: 14),
            OutlinedButton.icon(
              onPressed: onRefresh,
              icon: const Icon(Icons.refresh),
              label: const Text('Retry discovery'),
            ),
          ],
        ),
      ),
    );
  }
}

class _GatewayCard extends ConsumerWidget {
  final Server server;
  final VoidCallback onDevices;
  final VoidCallback onSwitches;
  final VoidCallback onConfigure;

  const _GatewayCard({
    required this.server,
    required this.onDevices,
    required this.onSwitches,
    required this.onConfigure,
  });

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final statusAsync = ref.watch(gatewayStatusProvider(server.serverId));
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
      padding: const EdgeInsets.all(18),
      decoration: BoxDecoration(
        color: Colors.white,
        borderRadius: BorderRadius.circular(20),
        border: Border.all(color: const Color(0xFFE5ECEB)),
        boxShadow: [
          BoxShadow(
            color: Colors.black.withOpacity(0.05),
            blurRadius: 18,
            offset: const Offset(0, 8),
          ),
        ],
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Container(
                padding: const EdgeInsets.symmetric(
                  horizontal: 10,
                  vertical: 6,
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
              const Spacer(),
              Icon(Icons.router_outlined, color: Colors.black.withOpacity(0.3)),
            ],
          ),
          const SizedBox(height: 12),
          Text(
            'Gateway ${_displayId(server.serverId)}',
            style: const TextStyle(fontSize: 20, fontWeight: FontWeight.w700),
          ),
          const SizedBox(height: 4),
          Text(
            'IP ${server.ip}',
            style: TextStyle(color: Colors.black.withOpacity(0.55)),
          ),
          const SizedBox(height: 14),
          Wrap(
            spacing: 10,
            runSpacing: 10,
            children: [
              FilledButton.icon(
                onPressed: onDevices,
                icon: const Icon(Icons.wifi_tethering),
                label: const Text('Devices'),
              ),
              OutlinedButton.icon(
                onPressed: onSwitches,
                icon: const Icon(Icons.toggle_on_outlined),
                label: const Text('Switches'),
              ),
              TextButton.icon(
                onPressed: onConfigure,
                icon: const Icon(Icons.tune),
                label: const Text('Configure'),
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
