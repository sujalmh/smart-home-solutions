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
      colorScheme: const ColorScheme.light(
        primary: Color(0xFF0F7B7A),
        secondary: Color(0xFFEDA84A),
        surface: Color(0xFFFDFBF7),
        background: Color(0xFFF7F4EE),
      ),
      useMaterial3: true,
    );

    return MaterialApp(
      title: 'Smart Home',
      theme: baseTheme.copyWith(
        textTheme: GoogleFonts.spaceGroteskTextTheme(baseTheme.textTheme),
        scaffoldBackgroundColor: const Color(0xFFF7F4EE),
        appBarTheme: const AppBarTheme(
          backgroundColor: Color(0xFFF7F4EE),
          surfaceTintColor: Colors.transparent,
          elevation: 0,
          centerTitle: false,
        ),
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
