import '../../models/ai_chat.dart';
import '../api_client.dart';

class AIRepository {
  final ApiClient api;

  AIRepository({required this.api});

  Future<AIChatResponse> sendMessage({
    required String message,
    String conversationId = 'default',
    bool? confirm,
  }) async {
    final payload = <String, dynamic>{
      'message': message,
      'conversation_id': conversationId,
      'confirm': ?confirm,
    };
    final response = await api.postJson('/api/ai/chat', payload);
    return AIChatResponse.fromJson(response);
  }
}
