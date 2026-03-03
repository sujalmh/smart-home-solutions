import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:go_router/go_router.dart';

import '../../config/app_colors.dart';
import '../../config/app_decorations.dart';
import '../../state/providers.dart';
import '../../utils/id_utils.dart';

class RegisterScreen extends ConsumerWidget {
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
              'Use a gateway to discover devices and configure remote control.',
              style: TextStyle(color: context.colors.subtitle),
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
                    separatorBuilder: (_, _) => const SizedBox(height: 12),
                    itemBuilder: (_, index) {
                      final server = servers[index];
                      return _GatewayTile(
                        serverId: server.serverId,
                        ip: server.ip,
                        onConfigure: () => context.push(
                          '/configure/${server.serverId}',
                        ),
                        onDiscover: () => context.push(
                          '/devices/${server.serverId}',
                        ),
                      );
                    },
                  );
                },
                loading: () => const Center(child: CircularProgressIndicator()),
                error: (_, _) =>
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
    final c = context.colors;
    final statusAsync = ref.watch(gatewayStatusProvider(serverId));
    final statusLabel = statusAsync.when(
      data: (online) => online ? 'Online' : 'Offline',
      loading: () => 'Checking',
      error: (_, _) => 'Unknown',
    );
    final statusColor = statusAsync.when(
      data: (online) => online ? c.online : c.offline,
      loading: () => c.neutral,
      error: (_, _) => c.neutral,
    );

    return Container(
      padding: const EdgeInsets.all(16),
      decoration: AppDecorations.section(c),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Text(
                'Gateway ${displayId(serverId)}',
                style: const TextStyle(fontWeight: FontWeight.w700),
              ),
              const Spacer(),
              Container(
                padding: const EdgeInsets.symmetric(
                  horizontal: 10,
                  vertical: 4,
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
            ],
          ),
          const SizedBox(height: 6),
          Text(
            'IP $ip',
            style: TextStyle(color: c.subtitle),
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
