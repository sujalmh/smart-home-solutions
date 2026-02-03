import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import '../../state/providers.dart';

class RegisterScreen extends ConsumerStatefulWidget {
  static const routeName = '/register-device';

  const RegisterScreen({super.key});

  @override
  ConsumerState<RegisterScreen> createState() => _RegisterScreenState();
}

class _RegisterScreenState extends ConsumerState<RegisterScreen> {
  final _serverController = TextEditingController();
  final _clientController = TextEditingController();

  @override
  void dispose() {
    _serverController.dispose();
    _clientController.dispose();
    super.dispose();
  }

  Future<void> _register() async {
    final serverId = _serverController.text.trim();
    if (serverId.isEmpty) {
      _showMessage('Enter a server device ID.');
      return;
    }

    await ref.read(deviceRepositoryProvider).registerDevice(serverId: serverId);
    _showMessage('Registration request sent.');
  }

  Future<void> _verifyGateway() async {
    final serverId = _serverController.text.trim();
    if (serverId.isEmpty) {
      _showMessage('Enter a server device ID.');
      return;
    }

    final online = await ref
        .read(deviceRepositoryProvider)
        .verifyGateway(serverId: serverId);
    _showMessage(online ? 'Gateway is online.' : 'Gateway is offline.');
  }

  Future<void> _bindSlave() async {
    final serverId = _serverController.text.trim();
    final clientId = _clientController.text.trim();
    if (serverId.isEmpty || clientId.isEmpty) {
      _showMessage('Enter server ID and slave ID.');
      return;
    }

    await ref
        .read(deviceRepositoryProvider)
        .bindSlave(serverId: serverId, clientId: clientId);
    _showMessage('Bind request sent to gateway.');
  }

  void _showMessage(String message) {
    ScaffoldMessenger.of(
      context,
    ).showSnackBar(SnackBar(content: Text(message)));
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('Local Register')),
      body: Padding(
        padding: const EdgeInsets.all(20),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            const Text(
              'Register and bind devices with the gateway.',
              style: TextStyle(fontSize: 16),
            ),
            const SizedBox(height: 16),
            TextField(
              controller: _serverController,
              decoration: const InputDecoration(
                labelText: 'Server ID',
                hintText: 'RSW-1234 or 1234',
              ),
            ),
            const SizedBox(height: 20),
            FilledButton(onPressed: _register, child: const Text('Register')),
            const SizedBox(height: 12),
            OutlinedButton(
              onPressed: _verifyGateway,
              child: const Text('Verify Gateway'),
            ),
            const SizedBox(height: 20),
            TextField(
              controller: _clientController,
              decoration: const InputDecoration(
                labelText: 'Slave ID',
                hintText: 'RSW-5678 or 5678',
              ),
            ),
            const SizedBox(height: 12),
            FilledButton(
              onPressed: _bindSlave,
              child: const Text('Bind Slave'),
            ),
          ],
        ),
      ),
    );
  }
}
