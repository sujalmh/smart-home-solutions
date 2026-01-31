import '../../models/auth_token.dart';
import '../../models/user.dart';
import '../api_client.dart';

class AuthRepository {
  final ApiClient api;

  AuthRepository({required this.api});

  Future<User> register({
    required String emailId,
    required String password,
    required String deviceId,
  }) async {
    final response = await api.postJson('/api/auth/register', {
      'email_id': emailId,
      'pwd': password,
      'device_id': deviceId,
    });
    return User.fromJson(response);
  }

  Future<AuthToken> login({
    required String emailId,
    required String password,
    String? deviceId,
  }) async {
    final response = await api.postJson('/api/auth/login', {
      'email_id': emailId,
      'pwd': password,
      if (deviceId != null) 'device_id': deviceId,
    });
    return AuthToken.fromJson(response);
  }
}
