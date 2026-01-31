class AuthState {
  final bool isLoading;
  final String? token;
  final String? emailId;
  final String? deviceId;
  final String? errorMessage;

  const AuthState({
    this.isLoading = false,
    this.token,
    this.emailId,
    this.deviceId,
    this.errorMessage,
  });

  AuthState copyWith({
    bool? isLoading,
    String? token,
    String? emailId,
    String? deviceId,
    String? errorMessage,
  }) {
    return AuthState(
      isLoading: isLoading ?? this.isLoading,
      token: token ?? this.token,
      emailId: emailId ?? this.emailId,
      deviceId: deviceId ?? this.deviceId,
      errorMessage: errorMessage,
    );
  }
}
