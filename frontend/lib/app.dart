import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:google_fonts/google_fonts.dart';

import 'config/app_colors.dart';
import 'config/app_router.dart';

class SmartHomeApp extends ConsumerWidget {
  const SmartHomeApp({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final router = ref.watch(routerProvider);

    final baseTheme = ThemeData(
      colorScheme: const ColorScheme.light(
        primary: Color(0xFF0F7B7A),
        secondary: Color(0xFFEDA84A),
        surface: Color(0xFFFDFBF7),
      ),
      useMaterial3: true,
      extensions: const <ThemeExtension<dynamic>>[AppColors.light],
    );

    return MaterialApp.router(
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
      routerConfig: router,
    );
  }
}
