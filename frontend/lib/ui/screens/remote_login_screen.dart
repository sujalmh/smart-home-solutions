import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import '../../state/providers.dart';
import 'room_switches_screen.dart';

class RemoteLoginScreen extends ConsumerStatefulWidget {
  static const routeName = '/remote-login';

  const RemoteLoginScreen({super.key});

  @override
  ConsumerState<RemoteLoginScreen> createState() => _RemoteLoginScreenState();
}

class _RemoteLoginScreenState extends ConsumerState<RemoteLoginScreen> {
  final _emailController = TextEditingController();
  final _passwordController = TextEditingController();
  String? _selectedServer;

  @override
  void dispose() {
    _emailController.dispose();
    _passwordController.dispose();
    super.dispose();
  }

  Future<void> _login() async {
    final email = _emailController.text.trim();
    final password = _passwordController.text.trim();
    if (email.isEmpty || password.isEmpty || _selectedServer == null) {
      _showMessage('Provide email, password, and a server.');
      return;
    }

    await ref
        .read(authControllerProvider.notifier)
        .login(emailId: email, password: password, deviceId: _selectedServer);

    final state = ref.read(authControllerProvider);
    if (state.errorMessage == null) {
      if (!mounted) return;
      Navigator.push(
        context,
        MaterialPageRoute(
          builder: (_) => RoomSwitchesScreen(serverId: _selectedServer!),
        ),
      );
    } else {
      _showMessage(state.errorMessage!);
    }
  }

  Future<void> _register() async {
    final email = _emailController.text.trim();
    final password = _passwordController.text.trim();
    if (email.isEmpty || password.isEmpty || _selectedServer == null) {
      _showMessage('Provide email, password, and a server.');
      return;
    }

    await ref
        .read(authControllerProvider.notifier)
        .register(
          emailId: email,
          password: password,
          deviceId: _selectedServer!,
        );

    final state = ref.read(authControllerProvider);
    if (state.errorMessage == null) {
      _showMessage('Registered and logged in.');
    } else {
      _showMessage(state.errorMessage!);
    }
  }

  void _showMessage(String message) {
    ScaffoldMessenger.of(
      context,
    ).showSnackBar(SnackBar(content: Text(message)));
  }

  @override
  Widget build(BuildContext context) {
    final authState = ref.watch(authControllerProvider);
    final serversAsync = ref.watch(serversProvider);

    return Scaffold(
      appBar: AppBar(title: const Text('Remote Login')),
      body: Padding(
        padding: const EdgeInsets.all(20),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            const Text(
              'Authenticate with your remote account.',
              style: TextStyle(fontSize: 16),
            ),
            const SizedBox(height: 16),
            TextField(
              controller: _emailController,
              decoration: const InputDecoration(labelText: 'Email'),
              keyboardType: TextInputType.emailAddress,
            ),
            const SizedBox(height: 12),
            TextField(
              controller: _passwordController,
              decoration: const InputDecoration(labelText: 'Password'),
              obscureText: true,
            ),
            const SizedBox(height: 12),
            serversAsync.when(
              data: (servers) {
                final items = servers
                    .map(
                      (server) => DropdownMenuItem<String>(
                        value: server.serverId,
                        child: Text(server.serverId),
                      ),
                    )
                    .toList();
                return DropdownButtonFormField<String>(
                  value: _selectedServer,
                  decoration: const InputDecoration(labelText: 'Server ID'),
                  items: items,
                  onChanged: (value) => setState(() => _selectedServer = value),
                );
              },
              loading: () => const Padding(
                padding: EdgeInsets.symmetric(vertical: 8),
                child: LinearProgressIndicator(),
              ),
              error: (_, __) => const Text('Unable to load servers.'),
            ),
            const SizedBox(height: 24),
            Row(
              children: [
                Expanded(
                  child: FilledButton(
                    onPressed: authState.isLoading ? null : _login,
                    child: const Text('Login'),
                  ),
                ),
                const SizedBox(width: 12),
                Expanded(
                  child: OutlinedButton(
                    onPressed: authState.isLoading ? null : _register,
                    child: const Text('Register'),
                  ),
                ),
              ],
            ),
          ],
        ),
      ),
    );
  }
}
