import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:go_router/go_router.dart';

import '../../config/app_colors.dart';
import '../../config/app_decorations.dart';
import '../../config/app_router.dart';
import '../../models/server.dart';
import '../../state/providers.dart';
import '../../utils/id_utils.dart';

class HomeScreen extends ConsumerWidget {
  const HomeScreen({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final serversAsync = ref.watch(serversProvider);
    final c = context.colors;

    // Fetch (or re-fetch) gateway status whenever the server list resolves.
    ref.listen(serversProvider, (_, next) {
      next.whenData((servers) {
        final notifier = ref.read(gatewayStatusNotifierProvider.notifier);
        for (final s in servers) {
          notifier.refresh(s.serverId);
        }
      });
    });

    return Scaffold(
      body: Container(
        decoration: BoxDecoration(
          gradient: AppDecorations.backgroundGradient(c),
        ),
        child: SafeArea(
          child: Padding(
            padding: const EdgeInsets.fromLTRB(20, 16, 20, 12),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                _Header(
                  onLogout: () =>
                      ref.read(authControllerProvider.notifier).logout(),
                  onAssistant: () => context.push(AppRoutes.assistant),
                  onRooms: () => context.push(AppRoutes.rooms),
                  onRefresh: () {
                    ref.refresh(serversProvider);
                    // Also immediately re-poll status for already-known servers.
                    final servers = serversAsync.valueOrNull;
                    if (servers != null) {
                      final notifier =
                          ref.read(gatewayStatusNotifierProvider.notifier);
                      for (final s in servers) {
                        notifier.refresh(s.serverId);
                      }
                    }
                  },
                ),
                const SizedBox(height: 16),
                Text(
                  'Gateways linked to your account',
                  style: TextStyle(
                    fontSize: 18,
                    fontWeight: FontWeight.w700,
                    color: c.heading,
                  ),
                ),
                const SizedBox(height: 6),
                Text(
                  'Remote control works when your gateway is online and reachable by backend.',
                  style: TextStyle(color: c.subtitle),
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
                        separatorBuilder: (_, _) => const SizedBox(height: 14),
                        itemBuilder: (_, index) => _GatewayCard(
                          server: servers[index],
                          onConfigure: () => context.push(
                            '/configure/${servers[index].serverId}',
                          ),
                          onDevices: () => context.push(
                            '/devices/${servers[index].serverId}',
                          ),
                          onSwitches: () => context.push(
                            '/switches/${servers[index].serverId}',
                          ),
                        ),
                      );
                    },
                    loading: () =>
                        const Center(child: CircularProgressIndicator()),
                    error: (_, _) =>
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
  final VoidCallback onLogout;
  final VoidCallback onAssistant;
  final VoidCallback onRooms;
  final VoidCallback onRefresh;

  const _Header({
    required this.onLogout,
    required this.onAssistant,
    required this.onRooms,
    required this.onRefresh,
  });

  @override
  Widget build(BuildContext context) {
    final c = context.colors;
    return Row(
      children: [
        Expanded(
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
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
                style: TextStyle(fontSize: 15, color: c.subtitle),
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
          onPressed: onLogout,
          icon: const Icon(Icons.logout),
          tooltip: 'Logout',
        ),
        IconButton(
          onPressed: onAssistant,
          icon: const Icon(Icons.smart_toy_outlined),
          tooltip: 'Assistant',
        ),
        IconButton(
          onPressed: onRooms,
          icon: const Icon(Icons.grid_view_outlined),
          tooltip: 'Rooms',
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
    final c = context.colors;
    return Center(
      child: Container(
        padding: const EdgeInsets.all(20),
        decoration: AppDecorations.card(c, radius: 20),
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            Icon(Icons.router_outlined, size: 42, color: c.primary),
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
    final c = context.colors;
    // bool? — null = not yet fetched ("Checking"), true/false = live status.
    final online = ref.watch(gatewayStatusProvider(server.serverId));
    final statusLabel =
        online == null ? 'Checking' : (online ? 'Online' : 'Offline');
    final statusColor =
        online == null ? c.neutral : (online ? c.online : c.offline);

    return Container(
      padding: const EdgeInsets.all(18),
      decoration: AppDecorations.card(c, radius: 20),
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
                decoration: AppDecorations.statusChip(statusColor),
                child: Text(
                  statusLabel,
                  style: TextStyle(
                    fontWeight: FontWeight.w600,
                    color: statusColor,
                  ),
                ),
              ),
              const Spacer(),
              Icon(
                Icons.router_outlined,
                color: Colors.black.withValues(alpha: 0.3),
              ),
            ],
          ),
          const SizedBox(height: 12),
          Text(
            'Gateway ${displayId(server.serverId)}',
            style: const TextStyle(fontSize: 20, fontWeight: FontWeight.w700),
          ),
          const SizedBox(height: 4),
          Text('IP ${server.ip}', style: TextStyle(color: c.subtitle)),
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
