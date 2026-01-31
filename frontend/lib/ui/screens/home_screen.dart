import 'package:flutter/material.dart';

import 'room_switches_screen.dart';
import 'remote_login_screen.dart';
import 'register_screen.dart';
import 'select_device_screen.dart';

class HomeScreen extends StatelessWidget {
  const HomeScreen({super.key});

  @override
  Widget build(BuildContext context) {
    final gradient = const LinearGradient(
      colors: [Color(0xFFF6F3ED), Color(0xFFE3EFEF)],
      begin: Alignment.topLeft,
      end: Alignment.bottomRight,
    );

    return Scaffold(
      body: Container(
        decoration: BoxDecoration(gradient: gradient),
        child: SafeArea(
          child: Padding(
            padding: const EdgeInsets.fromLTRB(20, 24, 20, 16),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                const Text(
                  'Smart Home',
                  style: TextStyle(
                    fontSize: 32,
                    fontWeight: FontWeight.w700,
                    letterSpacing: -0.5,
                  ),
                ),
                const SizedBox(height: 8),
                Text(
                  'Backend-driven control with real-time status updates.',
                  style: TextStyle(
                    color: Colors.black.withOpacity(0.6),
                    fontSize: 16,
                  ),
                ),
                const SizedBox(height: 24),
                Expanded(
                  child: ListView(
                    children: [
                      _SectionCard(
                        title: 'Remote Access',
                        subtitle: 'Login, register, and manage remotely.',
                        icon: Icons.public_outlined,
                        onTap: () => Navigator.pushNamed(
                          context,
                          RemoteLoginScreen.routeName,
                        ),
                      ),
                      _SectionCard(
                        title: 'Local Register',
                        subtitle: 'Register a server device locally.',
                        icon: Icons.add_link_outlined,
                        onTap: () => Navigator.pushNamed(
                          context,
                          RegisterScreen.routeName,
                        ),
                      ),
                      _SectionCard(
                        title: 'Select Device',
                        subtitle: 'Choose a local device to configure.',
                        icon: Icons.wifi_find_outlined,
                        onTap: () => Navigator.pushNamed(
                          context,
                          SelectDeviceScreen.routeName,
                        ),
                      ),
                      _SectionCard(
                        title: 'Switch Modules',
                        subtitle: 'Control modules and dimming values.',
                        icon: Icons.tune_outlined,
                        onTap: () => _promptServer(context),
                      ),
                    ],
                  ),
                ),
              ],
            ),
          ),
        ),
      ),
    );
  }
}

class _SectionCard extends StatelessWidget {
  final String title;
  final String subtitle;
  final IconData icon;
  final VoidCallback? onTap;

  const _SectionCard({
    required this.title,
    required this.subtitle,
    required this.icon,
    this.onTap,
  });

  @override
  Widget build(BuildContext context) {
    return InkWell(
      onTap: onTap,
      borderRadius: BorderRadius.circular(16),
      child: Container(
        margin: const EdgeInsets.only(bottom: 12),
        padding: const EdgeInsets.all(16),
        decoration: BoxDecoration(
          color: Colors.white,
          borderRadius: BorderRadius.circular(16),
          boxShadow: [
            BoxShadow(
              color: Colors.black.withOpacity(0.05),
              blurRadius: 16,
              offset: const Offset(0, 8),
            ),
          ],
        ),
        child: Row(
          children: [
            Container(
              width: 44,
              height: 44,
              decoration: BoxDecoration(
                color: const Color(0xFFE5F4F3),
                borderRadius: BorderRadius.circular(12),
              ),
              child: Icon(icon, color: const Color(0xFF0E7C7B)),
            ),
            const SizedBox(width: 16),
            Expanded(
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text(
                    title,
                    style: const TextStyle(
                      fontSize: 18,
                      fontWeight: FontWeight.w600,
                    ),
                  ),
                  const SizedBox(height: 4),
                  Text(
                    subtitle,
                    style: TextStyle(
                      color: Colors.black.withOpacity(0.6),
                      fontSize: 14,
                    ),
                  ),
                ],
              ),
            ),
            const Icon(Icons.chevron_right, color: Color(0xFF9BA9A8)),
          ],
        ),
      ),
    );
  }
}

void _promptServer(BuildContext context) {
  final controller = TextEditingController();
  showDialog(
    context: context,
    builder: (dialogContext) => AlertDialog(
      title: const Text('Enter Server ID'),
      content: TextField(
        controller: controller,
        decoration: const InputDecoration(hintText: 'RSW-xxxx'),
      ),
      actions: [
        TextButton(
          onPressed: () => Navigator.pop(dialogContext),
          child: const Text('Cancel'),
        ),
        FilledButton(
          onPressed: () {
            final serverId = controller.text.trim();
            if (serverId.isEmpty) {
              return;
            }
            Navigator.pop(dialogContext);
            Navigator.push(
              context,
              MaterialPageRoute(
                builder: (_) => RoomSwitchesScreen(serverId: serverId),
              ),
            );
          },
          child: const Text('Continue'),
        ),
      ],
    ),
  );
}
