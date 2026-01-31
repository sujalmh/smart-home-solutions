import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import '../../state/providers.dart';
import 'switch_screen.dart';

class RoomSwitchesScreen extends ConsumerWidget {
  final String serverId;

  const RoomSwitchesScreen({super.key, required this.serverId});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final clientsAsync = ref.watch(clientsProvider(serverId));

    return Scaffold(
      appBar: AppBar(title: const Text('Room Switches')),
      body: clientsAsync.when(
        data: (clients) {
          if (clients.isEmpty) {
            return const Center(child: Text('No clients registered yet.'));
          }
          return ListView.separated(
            padding: const EdgeInsets.all(16),
            itemCount: clients.length,
            separatorBuilder: (_, __) => const SizedBox(height: 12),
            itemBuilder: (_, index) {
              final client = clients[index];
              return ListTile(
                tileColor: Colors.white,
                shape: RoundedRectangleBorder(
                  borderRadius: BorderRadius.circular(14),
                ),
                title: Text(client.clientId),
                subtitle: Text('IP ${client.ip}'),
                trailing: const Icon(Icons.chevron_right),
                onTap: () => Navigator.push(
                  context,
                  MaterialPageRoute(
                    builder: (_) => SwitchScreen(
                      serverId: serverId,
                      clientId: client.clientId,
                    ),
                  ),
                ),
              );
            },
          );
        },
        loading: () => const Center(child: CircularProgressIndicator()),
        error: (_, __) => const Center(child: Text('Failed to load clients.')),
      ),
    );
  }
}
