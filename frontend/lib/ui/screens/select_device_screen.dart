import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import '../../state/providers.dart';
import 'config_screen.dart';

class SelectDeviceScreen extends ConsumerWidget {
  static const routeName = '/select-device';

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
                    separatorBuilder: (_, __) => const SizedBox(height: 12),
                    itemBuilder: (_, index) {
                      final server = servers[index];
                      return ListTile(
                        tileColor: Colors.white,
                        shape: RoundedRectangleBorder(
                          borderRadius: BorderRadius.circular(16),
                        ),
                        title: Text('Gateway ${_displayId(server.serverId)}'),
                        subtitle: Text('IP ${server.ip}'),
                        trailing: const Icon(Icons.chevron_right),
                        onTap: () => Navigator.push(
                          context,
                          MaterialPageRoute(
                            builder: (_) =>
                                ConfigScreen(deviceId: server.serverId),
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

String _displayId(String value) {
  if (value.startsWith('RSW-')) {
    return value.substring(4);
  }
  return value;
}
