import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import '../../state/providers.dart';
import 'network_devices_screen.dart';

enum ConfigMode { server, client, both }

class ConfigScreen extends ConsumerStatefulWidget {
  final String deviceId;

  const ConfigScreen({super.key, required this.deviceId});

  @override
  ConsumerState<ConfigScreen> createState() => _ConfigScreenState();
}

class _ConfigScreenState extends ConsumerState<ConfigScreen> {
  ConfigMode _mode = ConfigMode.server;
  String? _selectedServer;
  String? _selectedClient;
  final _ssidController = TextEditingController();
  final _remotePasswordController = TextEditingController();
  final _clientPasswordController = TextEditingController();
  final _clientIpController = TextEditingController();

  @override
  void initState() {
    super.initState();
    _selectedServer = widget.deviceId;
  }

  @override
  void dispose() {
    _ssidController.dispose();
    _remotePasswordController.dispose();
    _clientPasswordController.dispose();
    _clientIpController.dispose();
    super.dispose();
  }

  Future<void> _configureServer() async {
    final serverId = _selectedServer;
    if (serverId == null || serverId.isEmpty) {
      _showMessage('Choose a gateway first.');
      return;
    }

    await ref
        .read(deviceRepositoryProvider)
        .configureServer(serverId: serverId);

    if (_ssidController.text.trim().isNotEmpty &&
        _remotePasswordController.text.trim().isNotEmpty) {
      await ref
          .read(deviceRepositoryProvider)
          .configureRemote(
            serverId: serverId,
            ssid: _ssidController.text.trim(),
            password: _remotePasswordController.text.trim(),
          );
    }

    _showMessage('Server configuration sent.');
  }

  Future<void> _configureClient() async {
    final serverId = _selectedServer;
    final clientId = _selectedClient;
    final ip = _clientIpController.text.trim();
    final pwd = _clientPasswordController.text.trim();
    if (serverId == null || clientId == null || ip.isEmpty || pwd.isEmpty) {
      _showMessage('Choose a client and provide IP and password.');
      return;
    }

    await ref
        .read(deviceRepositoryProvider)
        .configureClient(
          serverId: serverId,
          clientId: clientId,
          password: pwd,
          ip: ip,
        );

    _showMessage('Client configuration sent.');
  }

  Future<void> _configureBoth() async {
    await _configureServer();
    await _configureClient();
  }

  void _showMessage(String message) {
    ScaffoldMessenger.of(
      context,
    ).showSnackBar(SnackBar(content: Text(message)));
  }

  @override
  Widget build(BuildContext context) {
    final serversAsync = ref.watch(serversProvider);

    return Scaffold(
      appBar: AppBar(title: const Text('Configure Gateway')),
      body: ListView(
        padding: const EdgeInsets.all(20),
        children: [
          _SectionCard(
            title: 'Configuration Mode',
            child: SegmentedButton<ConfigMode>(
              segments: const [
                ButtonSegment(value: ConfigMode.server, label: Text('Server')),
                ButtonSegment(value: ConfigMode.client, label: Text('Client')),
                ButtonSegment(value: ConfigMode.both, label: Text('Both')),
              ],
              selected: {_mode},
              onSelectionChanged: (value) =>
                  setState(() => _mode = value.first),
            ),
          ),
          const SizedBox(height: 16),
          _SectionCard(
            title: 'Gateway',
            child: serversAsync.when(
              data: (servers) {
                if (servers.isEmpty) {
                  return const Text('No gateways discovered yet.');
                }
                final fallback = servers.first.serverId;
                final selected = _selectedServer ?? fallback;
                if (_selectedServer == null) {
                  WidgetsBinding.instance.addPostFrameCallback((_) {
                    if (mounted) {
                      setState(() => _selectedServer = selected);
                    }
                  });
                }
                return DropdownButtonFormField<String>(
                  value: selected,
                  items: servers
                      .map(
                        (server) => DropdownMenuItem<String>(
                          value: server.serverId,
                          child: Text('Gateway ${_displayId(server.serverId)}'),
                        ),
                      )
                      .toList(),
                  decoration: const InputDecoration(labelText: 'Gateway'),
                  onChanged: (value) {
                    setState(() {
                      _selectedServer = value;
                      _selectedClient = null;
                    });
                  },
                );
              },
              loading: () => const LinearProgressIndicator(),
              error: (_, __) => const Text('Unable to load gateways.'),
            ),
          ),
          if (_mode != ConfigMode.client) ...[
            const SizedBox(height: 16),
            _SectionCard(
              title: 'Remote WiFi',
              child: Column(
                children: [
                  TextField(
                    controller: _ssidController,
                    decoration: const InputDecoration(labelText: 'SSID'),
                  ),
                  const SizedBox(height: 12),
                  TextField(
                    controller: _remotePasswordController,
                    decoration: const InputDecoration(labelText: 'Password'),
                    obscureText: true,
                  ),
                ],
              ),
            ),
          ],
          if (_mode != ConfigMode.server) ...[
            const SizedBox(height: 16),
            _SectionCard(
              title: 'Client Device',
              child: _ClientPicker(
                serverId: _selectedServer,
                selectedClient: _selectedClient,
                onSelected: (value) {
                  setState(() => _selectedClient = value);
                  if (value != null &&
                      _clientPasswordController.text.trim().isEmpty &&
                      value.startsWith('RSW-')) {
                    _clientPasswordController.text = value.substring(4);
                  }
                },
              ),
            ),
            const SizedBox(height: 16),
            _SectionCard(
              title: 'Client Network',
              child: Column(
                children: [
                  TextField(
                    controller: _clientPasswordController,
                    decoration: const InputDecoration(
                      labelText: 'Client Password',
                    ),
                    obscureText: true,
                  ),
                  const SizedBox(height: 12),
                  TextField(
                    controller: _clientIpController,
                    decoration: const InputDecoration(labelText: 'Client IP'),
                  ),
                ],
              ),
            ),
          ],
          const SizedBox(height: 20),
          if (_mode == ConfigMode.server)
            FilledButton(
              onPressed: _configureServer,
              child: const Text('Send Server Config'),
            )
          else if (_mode == ConfigMode.client)
            FilledButton(
              onPressed: _configureClient,
              child: const Text('Send Client Config'),
            )
          else
            FilledButton(
              onPressed: _configureBoth,
              child: const Text('Send Both Configs'),
            ),
        ],
      ),
    );
  }
}

class _ClientPicker extends ConsumerWidget {
  final String? serverId;
  final String? selectedClient;
  final ValueChanged<String?> onSelected;

  const _ClientPicker({
    required this.serverId,
    required this.selectedClient,
    required this.onSelected,
  });

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final resolvedServer = serverId;
    if (resolvedServer == null) {
      return const Text('Select a gateway to see its devices.');
    }

    final clientsAsync = ref.watch(clientsProvider(resolvedServer));
    return clientsAsync.when(
      data: (clients) {
        if (clients.isEmpty) {
          return Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              const Text('No clients registered yet.'),
              const SizedBox(height: 10),
              OutlinedButton.icon(
                onPressed: () => Navigator.push(
                  context,
                  MaterialPageRoute(
                    builder: (_) =>
                        NetworkDevicesScreen(serverId: resolvedServer),
                  ),
                ),
                icon: const Icon(Icons.wifi_tethering),
                label: const Text('Discover devices'),
              ),
            ],
          );
        }
        final fallback = clients.first.clientId;
        final selected = selectedClient ?? fallback;
        if (selectedClient == null) {
          WidgetsBinding.instance.addPostFrameCallback((_) {
            onSelected(selected);
          });
        }
        return DropdownButtonFormField<String>(
          value: selected,
          items: clients
              .map(
                (client) => DropdownMenuItem<String>(
                  value: client.clientId,
                  child: Text('Slave ${_displayId(client.clientId)}'),
                ),
              )
              .toList(),
          decoration: const InputDecoration(labelText: 'Client Device'),
          onChanged: onSelected,
        );
      },
      loading: () => const LinearProgressIndicator(),
      error: (_, __) => const Text('Unable to load clients.'),
    );
  }
}

class _SectionCard extends StatelessWidget {
  final String title;
  final Widget child;

  const _SectionCard({required this.title, required this.child});

  @override
  Widget build(BuildContext context) {
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
          Text(title, style: const TextStyle(fontWeight: FontWeight.w600)),
          const SizedBox(height: 12),
          child,
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
