import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import '../../config/app_colors.dart';
import '../../config/app_decorations.dart';
import '../../state/providers.dart';

class RemoteLoginScreen extends ConsumerStatefulWidget {
  const RemoteLoginScreen({super.key});

  @override
  ConsumerState<RemoteLoginScreen> createState() => _RemoteLoginScreenState();
}

class _RemoteLoginScreenState extends ConsumerState<RemoteLoginScreen> {
  final _emailController = TextEditingController();
  final _passwordController = TextEditingController();

  @override
  void dispose() {
    _emailController.dispose();
    _passwordController.dispose();
    super.dispose();
  }

  Future<void> _login() async {
    final email = _emailController.text.trim();
    final password = _passwordController.text.trim();
    if (email.isEmpty || password.isEmpty) {
      _showMessage('Provide email and password.');
      return;
    }

    await ref
        .read(authControllerProvider.notifier)
        .login(emailId: email, password: password);

    final state = ref.read(authControllerProvider);
    if (state.errorMessage == null) {
      return;
    } else {
      _showMessage(state.errorMessage!);
    }
  }

  Future<void> _register() async {
    final email = _emailController.text.trim();
    final password = _passwordController.text.trim();
    if (email.isEmpty || password.isEmpty) {
      _showMessage('Provide email and password.');
      return;
    }

    await ref
        .read(authControllerProvider.notifier)
        .register(emailId: email, password: password);

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
    final c = context.colors;

    return Scaffold(
      appBar: AppBar(title: const Text('Sign In')),
      body: Container(
        decoration: BoxDecoration(
          gradient: AppDecorations.backgroundGradient(c),
        ),
        child: Padding(
          padding: const EdgeInsets.all(20),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              const Text(
                'Authenticate to continue.',
                style: TextStyle(fontSize: 18, fontWeight: FontWeight.w700),
              ),
              const SizedBox(height: 6),
              Text(
                'Control your home remotely from any network once signed in.',
                style: TextStyle(color: c.subtitle),
              ),
              const SizedBox(height: 20),
              Container(
                padding: const EdgeInsets.all(16),
                decoration: AppDecorations.section(c),
                child: Column(
                  children: [
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
                  ],
                ),
              ),
              const SizedBox(height: 20),
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
              if (authState.errorMessage != null) ...[
                const SizedBox(height: 12),
                Text(
                  authState.errorMessage!,
                  style: TextStyle(color: c.error),
                ),
              ],
            ],
          ),
        ),
      ),
    );
  }
}
