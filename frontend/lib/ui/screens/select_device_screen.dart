import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:go_router/go_router.dart';

import '../../state/providers.dart';
import '../../utils/id_utils.dart';

class SelectDeviceScreen extends ConsumerWidget {
  const SelectDeviceScreen({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final serversAsync = ref.watch(serversProvider);

    return Scaffold(
      appBar: AppBar(
        title: const Text('Choose Gateway'),
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
              'Select a gateway to configure.',
              style: TextStyle(fontSize: 16, fontWeight: FontWeight.w600),
            ),
            const SizedBox(height: 16),
            Expanded(
              child: serversAsync.when(
                data: (servers) {
                  if (servers.isEmpty) {
                    return const Center(
                      child: Text('No gateways discovered yet.'),
                    );
                  }
                  return ListView.separated(
                    itemCount: servers.length,
                    separatorBuilder: (_, _) => const SizedBox(height: 12),
                    itemBuilder: (_, index) {
                      final server = servers[index];
                      return ListTile(
                        tileColor: Colors.white,
                        shape: RoundedRectangleBorder(
                          borderRadius: BorderRadius.circular(16),
                        ),
                        title: Text('Gateway ${displayId(server.serverId)}'),
                        subtitle: Text('IP ${server.ip}'),
                        trailing: const Icon(Icons.chevron_right),
                        onTap: () =>
                            context.push('/configure/${server.serverId}'),
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
