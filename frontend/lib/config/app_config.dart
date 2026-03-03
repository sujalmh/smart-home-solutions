/// Set to `true` to connect to the local backend (192.168.0.109:8000).
/// Set to `false` to use the production cloud server.
const bool kUseLocal = true;

class AppConfig {
  final String baseUrl;
  final String socketUrl;
  final String socketPath;

  const AppConfig({
    required this.baseUrl,
    required this.socketUrl,
    required this.socketPath,
  });

  factory AppConfig.local() {
    return const AppConfig(
      baseUrl: 'http://192.168.0.109:8000',
      socketUrl: 'http://192.168.0.109:8000',
      socketPath: '/ws',
    );
  }

  factory AppConfig.production() {
    const baseUrl = String.fromEnvironment(
      'SMART_HOME_BASE_URL',
      defaultValue: 'https://api.sms.hebbit.tech',
    );
    const socketUrlEnv = String.fromEnvironment(
      'SMART_HOME_SOCKET_URL',
      defaultValue: '',
    );
    const socketPath = String.fromEnvironment(
      'SMART_HOME_SOCKET_PATH',
      defaultValue: '/ws',
    );
    final socketUrl = socketUrlEnv.isEmpty ? baseUrl : socketUrlEnv;
    return AppConfig(
      baseUrl: baseUrl,
      socketUrl: socketUrl,
      socketPath: socketPath,
    );
  }
}
