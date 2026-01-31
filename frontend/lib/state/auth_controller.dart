import 'package:flutter_riverpod/flutter_riverpod.dart';

import '../data/repositories/auth_repository.dart';
import '../data/socket_client.dart';
import 'auth_state.dart';

class AuthController extends StateNotifier<AuthState> {
  final AuthRepository authRepository;
  final SocketClient socketClient;

  AuthController({required this.authRepository, required this.socketClient})
    : super(const AuthState());

  Future<void> register({
    required String emailId,
    required String password,
    required String deviceId,
  }) async {
    state = state.copyWith(isLoading: true, errorMessage: null);
    try {
      await authRepository.register(
        emailId: emailId,
        password: password,
        deviceId: deviceId,
      );
      await login(emailId: emailId, password: password, deviceId: deviceId);
    } catch (error) {
      state = state.copyWith(isLoading: false, errorMessage: error.toString());
    }
  }

  Future<void> login({
    required String emailId,
    required String password,
    String? deviceId,
  }) async {
    state = state.copyWith(isLoading: true, errorMessage: null);
    try {
      final token = await authRepository.login(
        emailId: emailId,
        password: password,
        deviceId: deviceId,
      );
      socketClient.connect(token: token.accessToken);
      state = state.copyWith(
        isLoading: false,
        token: token.accessToken,
        emailId: emailId,
        deviceId: deviceId,
      );
    } catch (error) {
      state = state.copyWith(isLoading: false, errorMessage: error.toString());
    }
  }
}
