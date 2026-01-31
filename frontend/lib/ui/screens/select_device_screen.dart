import 'package:flutter/material.dart';

import 'config_screen.dart';

class SelectDeviceScreen extends StatefulWidget {
  static const routeName = '/select-device';

  const SelectDeviceScreen({super.key});

  @override
  State<SelectDeviceScreen> createState() => _SelectDeviceScreenState();
}

class _SelectDeviceScreenState extends State<SelectDeviceScreen> {
  final _deviceController = TextEditingController();

  @override
  void dispose() {
    _deviceController.dispose();
    super.dispose();
  }

  void _continue() {
    final deviceId = _deviceController.text.trim();
    if (deviceId.isEmpty) {
      ScaffoldMessenger.of(
        context,
      ).showSnackBar(const SnackBar(content: Text('Enter a device SSID.')));
      return;
    }

    Navigator.push(
      context,
      MaterialPageRoute(builder: (_) => ConfigScreen(deviceId: deviceId)),
    );
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('Select Device')),
      body: Padding(
        padding: const EdgeInsets.all(20),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            const Text(
              'Enter the device SSID (RSW-xxxx).',
              style: TextStyle(fontSize: 16),
            ),
            const SizedBox(height: 16),
            TextField(
              controller: _deviceController,
              decoration: const InputDecoration(labelText: 'Device SSID'),
            ),
            const SizedBox(height: 20),
            FilledButton(onPressed: _continue, child: const Text('Continue')),
          ],
        ),
      ),
    );
  }
}
