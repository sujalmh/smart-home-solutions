import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:go_router/go_router.dart';

import '../state/providers.dart';
import '../ui/screens/assistant_panel_screen.dart';
import '../ui/screens/config_screen.dart';
import '../ui/screens/home_screen.dart';
import '../ui/screens/network_devices_screen.dart';
import '../ui/screens/register_screen.dart';
import '../ui/screens/remote_login_screen.dart';
import '../ui/screens/room_switches_screen.dart';
import '../ui/screens/rooms_dashboard_screen.dart';
import '../ui/screens/select_device_screen.dart';
import '../ui/screens/switch_screen.dart';

/// Named route paths.
abstract final class AppRoutes {
  static const login = '/login';
  static const home = '/';
  static const assistant = '/assistant';
  static const register = '/register-device';
  static const selectDevice = '/select-device';
  static const rooms = '/rooms';
  static const configure = '/configure/:deviceId';
  static const devices = '/devices/:serverId';
  static const switches = '/switches/:serverId';
  static const switchControl = '/switch/:serverId/:clientId';
}

final routerProvider = Provider<GoRouter>((ref) {
  final authState = ref.watch(authControllerProvider);

  return GoRouter(
    refreshListenable: _AuthNotifier(ref),
    redirect: (context, state) {
      final isLoggedIn = authState.isAuthenticated;
      final isOnLogin = state.matchedLocation == AppRoutes.login;

      if (!isLoggedIn && !isOnLogin) return AppRoutes.login;
      if (isLoggedIn && isOnLogin) return AppRoutes.home;
      return null;
    },
    routes: [
      GoRoute(
        path: AppRoutes.login,
        builder: (_, _) => const RemoteLoginScreen(),
      ),
      GoRoute(path: AppRoutes.home, builder: (_, _) => const HomeScreen()),
      GoRoute(
        path: AppRoutes.assistant,
        builder: (_, _) => const AssistantPanelScreen(),
      ),
      GoRoute(
        path: AppRoutes.register,
        builder: (_, _) => const RegisterScreen(),
      ),
      GoRoute(
        path: AppRoutes.selectDevice,
        builder: (_, _) => const SelectDeviceScreen(),
      ),
      GoRoute(
        path: AppRoutes.rooms,
        builder: (_, _) => const RoomsDashboardScreen(),
      ),
      GoRoute(
        path: AppRoutes.configure,
        builder: (_, state) =>
            ConfigScreen(deviceId: state.pathParameters['deviceId']!),
      ),
      GoRoute(
        path: AppRoutes.devices,
        builder: (_, state) =>
            NetworkDevicesScreen(serverId: state.pathParameters['serverId']!),
      ),
      GoRoute(
        path: AppRoutes.switches,
        builder: (_, state) =>
            RoomSwitchesScreen(serverId: state.pathParameters['serverId']!),
      ),
      GoRoute(
        path: AppRoutes.switchControl,
        builder: (_, state) {
          final extra = state.extra as Map<String, dynamic>?;
          return SwitchScreen(
            serverId: state.pathParameters['serverId']!,
            clientId: state.pathParameters['clientId']!,
            moduleCount: extra?['moduleCount'] as int? ?? 4,
          );
        },
      ),
    ],
  );
});

/// Bridges Riverpod auth state changes into GoRouter's [Listenable] refresh.
class _AuthNotifier extends ChangeNotifier {
  _AuthNotifier(this._ref) {
    _ref.listen(authControllerProvider, (_, _) => notifyListeners());
  }

  final Ref _ref;
}
