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

  @override
  void dispose() {
    _serverController.dispose();
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
              'Register a server device with the backend.',
              style: TextStyle(fontSize: 16),
            ),
            const SizedBox(height: 16),
            TextField(
              controller: _serverController,
              decoration: const InputDecoration(labelText: 'Server ID'),
            ),
            const SizedBox(height: 20),
            FilledButton(onPressed: _register, child: const Text('Register')),
            const SizedBox(height: 12),
            OutlinedButton(
              onPressed: _verifyGateway,
              child: const Text('Verify Gateway'),
            ),
          ],
        ),
      ),
    );
  }
}
