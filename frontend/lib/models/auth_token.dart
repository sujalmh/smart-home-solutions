class AuthToken {
  final String accessToken;
  final String tokenType;

  const AuthToken({required this.accessToken, required this.tokenType});

  factory AuthToken.fromJson(Map<String, dynamic> json) {
    return AuthToken(
      accessToken: json['access_token'] as String,
      tokenType: (json['token_type'] as String?) ?? 'bearer',
    );
  }
}
