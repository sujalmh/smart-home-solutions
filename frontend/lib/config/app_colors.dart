import 'package:flutter/material.dart';

/// Centralised colour palette exposed as a [ThemeExtension] so every widget
/// can read colours from `Theme.of(context).extension<AppColors>()!` rather
/// than scattering hex literals throughout the tree.
@immutable
class AppColors extends ThemeExtension<AppColors> {
  // ── Brand ──────────────────────────────────────────────────────────────
  final Color primary;
  final Color secondary;

  // ── Status ─────────────────────────────────────────────────────────────
  final Color online;
  final Color offline;
  final Color neutral;
  final Color error;
  final Color errorBg;
  final Color errorBorder;

  // ── Surface / UI chrome ────────────────────────────────────────────────
  final Color cardBorder;
  final Color composerBorder;
  final Color chipBg;
  final Color surfaceAlt;

  // ── Text / Opacity helpers ─────────────────────────────────────────────
  final Color heading;
  final Color subtitle;

  // ── Gradient endpoints ─────────────────────────────────────────────────
  final Color gradientStart;
  final Color gradientEnd;

  // ── Switch‑card specific ───────────────────────────────────────────────
  final Color rateLimitBg;
  final Color staleBg;

  const AppColors({
    required this.primary,
    required this.secondary,
    required this.online,
    required this.offline,
    required this.neutral,
    required this.error,
    required this.errorBg,
    required this.errorBorder,
    required this.cardBorder,
    required this.composerBorder,
    required this.chipBg,
    required this.surfaceAlt,
    required this.heading,
    required this.subtitle,
    required this.gradientStart,
    required this.gradientEnd,
    required this.rateLimitBg,
    required this.staleBg,
  });

  /// Default light‑mode palette extracted from the existing codebase.
  static const light = AppColors(
    primary: Color(0xFF0F7B7A),
    secondary: Color(0xFFEDA84A),
    online: Color(0xFF1E9E7A),
    offline: Color(0xFFE27D60),
    neutral: Color(0xFF7A8C8B),
    error: Color(0xFF8A3D28),
    errorBg: Color(0xFFFFEFEA),
    errorBorder: Color(0xFFF0BAA8),
    cardBorder: Color(0xFFE5ECEB),
    composerBorder: Color(0xFFD9E4E3),
    chipBg: Color(0xFFE7F5F4),
    surfaceAlt: Color(0xFFF5F7F7),
    heading: Color(0xFF101414),
    subtitle: Color(0xFF5C6B6B),
    gradientStart: Color(0xFFF7F4EE),
    gradientEnd: Color(0xFFE6F1F0),
    rateLimitBg: Color(0xFF9AB0D7),
    staleBg: Color(0xFFF2C26A),
  );

  @override
  AppColors copyWith({
    Color? primary,
    Color? secondary,
    Color? online,
    Color? offline,
    Color? neutral,
    Color? error,
    Color? errorBg,
    Color? errorBorder,
    Color? cardBorder,
    Color? composerBorder,
    Color? chipBg,
    Color? surfaceAlt,
    Color? heading,
    Color? subtitle,
    Color? gradientStart,
    Color? gradientEnd,
    Color? rateLimitBg,
    Color? staleBg,
  }) {
    return AppColors(
      primary: primary ?? this.primary,
      secondary: secondary ?? this.secondary,
      online: online ?? this.online,
      offline: offline ?? this.offline,
      neutral: neutral ?? this.neutral,
      error: error ?? this.error,
      errorBg: errorBg ?? this.errorBg,
      errorBorder: errorBorder ?? this.errorBorder,
      cardBorder: cardBorder ?? this.cardBorder,
      composerBorder: composerBorder ?? this.composerBorder,
      chipBg: chipBg ?? this.chipBg,
      surfaceAlt: surfaceAlt ?? this.surfaceAlt,
      heading: heading ?? this.heading,
      subtitle: subtitle ?? this.subtitle,
      gradientStart: gradientStart ?? this.gradientStart,
      gradientEnd: gradientEnd ?? this.gradientEnd,
      rateLimitBg: rateLimitBg ?? this.rateLimitBg,
      staleBg: staleBg ?? this.staleBg,
    );
  }

  @override
  AppColors lerp(AppColors? other, double t) {
    if (other is! AppColors) return this;
    return AppColors(
      primary: Color.lerp(primary, other.primary, t)!,
      secondary: Color.lerp(secondary, other.secondary, t)!,
      online: Color.lerp(online, other.online, t)!,
      offline: Color.lerp(offline, other.offline, t)!,
      neutral: Color.lerp(neutral, other.neutral, t)!,
      error: Color.lerp(error, other.error, t)!,
      errorBg: Color.lerp(errorBg, other.errorBg, t)!,
      errorBorder: Color.lerp(errorBorder, other.errorBorder, t)!,
      cardBorder: Color.lerp(cardBorder, other.cardBorder, t)!,
      composerBorder: Color.lerp(composerBorder, other.composerBorder, t)!,
      chipBg: Color.lerp(chipBg, other.chipBg, t)!,
      surfaceAlt: Color.lerp(surfaceAlt, other.surfaceAlt, t)!,
      heading: Color.lerp(heading, other.heading, t)!,
      subtitle: Color.lerp(subtitle, other.subtitle, t)!,
      gradientStart: Color.lerp(gradientStart, other.gradientStart, t)!,
      gradientEnd: Color.lerp(gradientEnd, other.gradientEnd, t)!,
      rateLimitBg: Color.lerp(rateLimitBg, other.rateLimitBg, t)!,
      staleBg: Color.lerp(staleBg, other.staleBg, t)!,
    );
  }
}

/// Convenience extension so callers can write `context.colors` instead of
/// `Theme.of(context).extension<AppColors>()!`.
extension AppColorsX on BuildContext {
  AppColors get colors => Theme.of(this).extension<AppColors>()!;
}
