import 'package:equatable/equatable.dart';

class User extends Equatable {
  final String emailId;
  final String? deviceId;

  const User({required this.emailId, required this.deviceId});

  factory User.fromJson(Map<String, dynamic> json) {
    return User(
      emailId: json['email_id'] as String,
      deviceId: json['device_id'] as String?,
    );
  }

  Map<String, dynamic> toJson() {
    return {'email_id': emailId, 'device_id': deviceId};
  }

  @override
  List<Object?> get props => [emailId, deviceId];
}
