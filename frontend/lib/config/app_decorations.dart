import 'package:flutter/material.dart';

import 'app_colors.dart';

// ── Radius constants ─────────────────────────────────────────────────────
const double kCardRadius = 18.0;
const double kSectionRadius = 16.0;
const double kChipRadius = 12.0;
const double kButtonRadius = 14.0;

/// Shared decoration helpers that keep surface styling consistent across
/// screens and reduce duplicated [BoxDecoration] literals.
class AppDecorations {
  AppDecorations._();

  /// The gradient used as a page background on most screens.
  static LinearGradient backgroundGradient(AppColors colors) {
    return LinearGradient(
      colors: [colors.gradientStart, colors.gradientEnd],
      begin: Alignment.topLeft,
      end: Alignment.bottomRight,
    );
  }

  /// Standard white card with a subtle border and optional shadow.
  static BoxDecoration card(
    AppColors colors, {
    double radius = kCardRadius,
    bool elevated = true,
  }) {
    return BoxDecoration(
      color: Colors.white,
      borderRadius: BorderRadius.circular(radius),
      border: Border.all(color: colors.cardBorder),
      boxShadow: elevated
          ? [
              BoxShadow(
                color: Colors.black.withValues(alpha: 0.05),
                blurRadius: 18,
                offset: const Offset(0, 8),
              ),
            ]
          : null,
    );
  }

  /// Flat section card (no shadow) — used for form groups.
  static BoxDecoration section(AppColors colors) {
    return BoxDecoration(
      color: Colors.white,
      borderRadius: BorderRadius.circular(kSectionRadius),
      border: Border.all(color: colors.cardBorder),
    );
  }

  /// Status chip decoration with a tinted background.
  static BoxDecoration statusChip(Color color) {
    return BoxDecoration(
      color: color.withValues(alpha: 0.12),
      borderRadius: BorderRadius.circular(999),
    );
  }
}
