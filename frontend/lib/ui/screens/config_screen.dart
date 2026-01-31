import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import '../../state/providers.dart';

enum ConfigMode { server, client, both }

class ConfigScreen extends ConsumerStatefulWidget {
  final String deviceId;

  const ConfigScreen({super.key, required this.deviceId});

  @override
  ConsumerState<ConfigScreen> createState() => _ConfigScreenState();
}

class _ConfigScreenState extends ConsumerState<ConfigScreen> {
  ConfigMode _mode = ConfigMode.server;
  late final TextEditingController _serverController;
  late final TextEditingController _clientController;
  final _ssidController = TextEditingController();
  final _remotePasswordController = TextEditingController();
  final _clientPasswordController = TextEditingController();
  final _clientIpController = TextEditingController();

  @override
  void initState() {
    super.initState();
    _serverController = TextEditingController(text: widget.deviceId);
    _clientController = TextEditingController(text: widget.deviceId);
    if (widget.deviceId.startsWith('RSW-') && widget.deviceId.length > 4) {
      _clientPasswordController.text = widget.deviceId.substring(4);
    }
  }

  @override
  void dispose() {
    _serverController.dispose();
    _clientController.dispose();
    _ssidController.dispose();
    _remotePasswordController.dispose();
    _clientPasswordController.dispose();
    _clientIpController.dispose();
    super.dispose();
  }

  Future<void> _configureServer() async {
    final serverId = _serverController.text.trim();
    if (serverId.isEmpty) {
      _showMessage('Server ID is required.');
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
    final serverId = _serverController.text.trim();
    final clientId = _clientController.text.trim();
    final ip = _clientIpController.text.trim();
    final pwd = _clientPasswordController.text.trim();
    if (serverId.isEmpty || clientId.isEmpty || ip.isEmpty || pwd.isEmpty) {
      _showMessage('Server ID, client ID, IP, and password are required.');
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
    return Scaffold(
      appBar: AppBar(title: const Text('Configure Device')),
      body: ListView(
        padding: const EdgeInsets.all(20),
        children: [
          SegmentedButton<ConfigMode>(
            segments: const [
              ButtonSegment(value: ConfigMode.server, label: Text('Server')),
              ButtonSegment(value: ConfigMode.client, label: Text('Client')),
              ButtonSegment(value: ConfigMode.both, label: Text('Both')),
            ],
            selected: {_mode},
            onSelectionChanged: (value) {
              setState(() => _mode = value.first);
            },
          ),
          const SizedBox(height: 20),
          TextField(
            controller: _serverController,
            decoration: const InputDecoration(labelText: 'Server ID'),
          ),
          const SizedBox(height: 12),
          if (_mode != ConfigMode.client) ...[
            TextField(
              controller: _ssidController,
              decoration: const InputDecoration(labelText: 'Remote WiFi SSID'),
            ),
            const SizedBox(height: 12),
            TextField(
              controller: _remotePasswordController,
              decoration: const InputDecoration(
                labelText: 'Remote WiFi Password',
              ),
              obscureText: true,
            ),
            const SizedBox(height: 12),
          ],
          if (_mode != ConfigMode.server) ...[
            TextField(
              controller: _clientController,
              decoration: const InputDecoration(labelText: 'Client ID'),
            ),
            const SizedBox(height: 12),
            TextField(
              controller: _clientPasswordController,
              decoration: const InputDecoration(labelText: 'Client Password'),
              obscureText: true,
            ),
            const SizedBox(height: 12),
            TextField(
              controller: _clientIpController,
              decoration: const InputDecoration(labelText: 'Client IP'),
            ),
            const SizedBox(height: 12),
          ],
          const SizedBox(height: 20),
          if (_mode == ConfigMode.server)
            FilledButton(
              onPressed: _configureServer,
              child: const Text('Configure Server'),
            )
          else if (_mode == ConfigMode.client)
            FilledButton(
              onPressed: _configureClient,
              child: const Text('Configure Client'),
            )
          else
            FilledButton(
              onPressed: _configureBoth,
              child: const Text('Configure Both'),
            ),
        ],
      ),
    );
  }
}
