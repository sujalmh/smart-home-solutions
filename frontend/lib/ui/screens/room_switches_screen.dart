import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:go_router/go_router.dart';

import '../../config/app_colors.dart';
import '../../config/app_decorations.dart';
import '../../state/providers.dart';
import '../../utils/id_utils.dart';

class RoomSwitchesScreen extends ConsumerWidget {
  final String serverId;

  const RoomSwitchesScreen({super.key, required this.serverId});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final clientsAsync = ref.watch(clientsProvider(serverId));
    final c = context.colors;

    return Scaffold(
      appBar: AppBar(title: const Text('Switch Modules')),
      body: Container(
        decoration: BoxDecoration(
          gradient: AppDecorations.backgroundGradient(c),
        ),
        child: clientsAsync.when(
          data: (clients) {
            if (clients.isEmpty) {
              return Center(
                child: Column(
                  mainAxisSize: MainAxisSize.min,
                  children: [
                    const Text('No devices bound yet.'),
                    const SizedBox(height: 12),
                    OutlinedButton.icon(
                      onPressed: () => context.push(
                        '/devices/$serverId',
                      ),
                      icon: const Icon(Icons.wifi_tethering),
                      label: const Text('Discover devices'),
                    ),
                  ],
                ),
              );
            }
            return ListView.separated(
              padding: const EdgeInsets.all(20),
              itemCount: clients.length,
              separatorBuilder: (_, _) => const SizedBox(height: 12),
              itemBuilder: (_, index) {
                final client = clients[index];
                return Container(
                  padding: const EdgeInsets.all(16),
                  decoration: AppDecorations.section(c),
                  child: Row(
                    children: [
                      Container(
                        width: 44,
                        height: 44,
                        decoration: BoxDecoration(
                          color: c.chipBg,
                          borderRadius: BorderRadius.circular(12),
                        ),
                        child: Icon(
                          Icons.toggle_on_outlined,
                          color: c.primary,
                        ),
                      ),
                      const SizedBox(width: 12),
                      Expanded(
                        child: Column(
                          crossAxisAlignment: CrossAxisAlignment.start,
                          children: [
                            Text(
                              'Slave ${displayId(client.clientId)}',
                              style: const TextStyle(
                                fontWeight: FontWeight.w700,
                                fontSize: 16,
                              ),
                            ),
                            const SizedBox(height: 4),
                            Text(
                              'IP ${client.ip}',
                              style: TextStyle(
                                color: c.subtitle,
                              ),
                            ),
                          ],
                        ),
                      ),
                      IconButton(
                        onPressed: () => context.push(
                          '/switch/$serverId/${client.clientId}',
                          extra: {'moduleCount': client.moduleCount},
                        ),
                        icon: const Icon(Icons.chevron_right),
                      ),
                    ],
                  ),
                );
              },
            );
          },
          loading: () => const Center(child: CircularProgressIndicator()),
          error: (_, _) => const Center(child: Text('Failed to load clients.')),
        ),
      ),
    );
  }
}
