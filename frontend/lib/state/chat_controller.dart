import 'package:flutter_riverpod/flutter_riverpod.dart';

import '../data/repositories/ai_repository.dart';
import '../models/ai_chat.dart';

/// Represents one chat bubble — either from the user or the assistant.
enum ChatRole { user, assistant }

class ChatMessage {
  final ChatRole role;
  final String text;
  final AIChatResponse? response;

  const ChatMessage({
    required this.role,
    required this.text,
    this.response,
  });

  factory ChatMessage.user(String text) =>
      ChatMessage(role: ChatRole.user, text: text);

  factory ChatMessage.assistant(AIChatResponse response) => ChatMessage(
        role: ChatRole.assistant,
        text: response.response.reply,
        response: response,
      );
}

class ChatState {
  final List<ChatMessage> messages;
  final bool sending;
  final String? error;

  const ChatState({
    this.messages = const [],
    this.sending = false,
    this.error,
  });

  ChatState copyWith({
    List<ChatMessage>? messages,
    bool? sending,
    String? error,
  }) {
    return ChatState(
      messages: messages ?? this.messages,
      sending: sending ?? this.sending,
      error: error,
    );
  }

  /// Returns the last assistant response that requires confirmation, if any.
  bool get needsConfirmation {
    for (final m in messages.reversed) {
      if (m.role == ChatRole.assistant && m.response != null) {
        return m.response!.response.requiresConfirmation;
      }
    }
    return false;
  }
}

class ChatController extends StateNotifier<ChatState> {
  static const _conversationId = 'default';
  final AIRepository _aiRepository;

  ChatController({required AIRepository aiRepository})
      : _aiRepository = aiRepository,
        super(const ChatState());

  Future<void> sendMessage(String text) async {
    if (text.trim().isEmpty || state.sending) return;

    state = state.copyWith(
      messages: [...state.messages, ChatMessage.user(text.trim())],
      sending: true,
      error: null,
    );

    try {
      final response = await _aiRepository.sendMessage(
        message: text.trim(),
        conversationId: _conversationId,
      );
      state = state.copyWith(
        messages: [...state.messages, ChatMessage.assistant(response)],
        sending: false,
      );
    } catch (e) {
      state = state.copyWith(sending: false, error: e.toString());
    }
  }

  Future<void> sendConfirmation(bool confirm) async {
    if (state.sending) return;

    state = state.copyWith(
      messages: [
        ...state.messages,
        ChatMessage.user(confirm ? 'Confirm action' : 'Cancel action'),
      ],
      sending: true,
      error: null,
    );

    try {
      final response = await _aiRepository.sendMessage(
        message: confirm ? 'confirm' : 'cancel',
        conversationId: _conversationId,
        confirm: confirm,
      );
      state = state.copyWith(
        messages: [...state.messages, ChatMessage.assistant(response)],
        sending: false,
      );
    } catch (e) {
      state = state.copyWith(sending: false, error: e.toString());
    }
  }
}
