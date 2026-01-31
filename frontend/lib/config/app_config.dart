class AppConfig {
  final String baseUrl;
  final String socketUrl;
  final String socketPath;

  const AppConfig({
    required this.baseUrl,
    required this.socketUrl,
    required this.socketPath,
  });

  factory AppConfig.production() {
    const baseUrl = String.fromEnvironment(
      'SMART_HOME_BASE_URL',
      defaultValue: 'http://127.0.0.1:8000',
    );
    const socketUrl = String.fromEnvironment(
      'SMART_HOME_SOCKET_URL',
      defaultValue: 'http://127.0.0.1:8000',
    );
    const socketPath = String.fromEnvironment(
      'SMART_HOME_SOCKET_PATH',
      defaultValue: '/socket.io',
    );
    return const AppConfig(
      baseUrl: baseUrl,
      socketUrl: socketUrl,
      socketPath: socketPath,
    );
  }
}
