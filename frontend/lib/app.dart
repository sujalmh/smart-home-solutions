import 'package:flutter/material.dart';
import 'package:google_fonts/google_fonts.dart';

import 'ui/screens/home_screen.dart';
import 'ui/screens/remote_login_screen.dart';
import 'ui/screens/register_screen.dart';
import 'ui/screens/select_device_screen.dart';

class SmartHomeApp extends StatelessWidget {
  const SmartHomeApp({super.key});

  @override
  Widget build(BuildContext context) {
    final baseTheme = ThemeData(
      colorScheme: ColorScheme.fromSeed(seedColor: const Color(0xFF0E7C7B)),
      useMaterial3: true,
    );

    return MaterialApp(
      title: 'Smart Home',
      theme: baseTheme.copyWith(
        textTheme: GoogleFonts.manropeTextTheme(baseTheme.textTheme),
        scaffoldBackgroundColor: const Color(0xFFF6F3ED),
      ),
      home: const HomeScreen(),
      routes: {
        RemoteLoginScreen.routeName: (_) => const RemoteLoginScreen(),
        RegisterScreen.routeName: (_) => const RegisterScreen(),
        SelectDeviceScreen.routeName: (_) => const SelectDeviceScreen(),
      },
    );
  }
}
