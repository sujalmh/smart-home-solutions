import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import '../../models/ai_chat.dart';
import '../../state/providers.dart';

class AssistantPanelScreen extends ConsumerStatefulWidget {
  static const routeName = '/assistant';

  const AssistantPanelScreen({super.key});

  @override
  ConsumerState<AssistantPanelScreen> createState() =>
      _AssistantPanelScreenState();
}

class _AssistantPanelScreenState extends ConsumerState<AssistantPanelScreen> {
  static const _conversationId = 'default';

  final TextEditingController _controller = TextEditingController();
  final ScrollController _scrollController = ScrollController();
  final List<_ChatMessage> _messages = <_ChatMessage>[];

  bool _sending = false;
  String? _error;

  @override
  void dispose() {
    _controller.dispose();
    _scrollController.dispose();
    super.dispose();
  }

  Future<void> _sendMessage() async {
    final text = _controller.text.trim();
    if (text.isEmpty || _sending) {
      return;
    }

    setState(() {
      _error = null;
      _sending = true;
      _messages.add(_ChatMessage.user(text));
      _controller.clear();
    });
    _scrollToBottom();

    try {
      final response = await ref
          .read(aiRepositoryProvider)
          .sendMessage(message: text, conversationId: _conversationId);
      setState(() {
        _messages.add(_ChatMessage.assistant(response));
      });
    } catch (error) {
      setState(() {
        _error = error.toString();
      });
    } finally {
      setState(() {
        _sending = false;
      });
      _scrollToBottom();
    }
  }

  Future<void> _sendConfirmation(bool confirm) async {
    if (_sending) {
      return;
    }
    setState(() {
      _sending = true;
      _error = null;
      _messages.add(
        _ChatMessage.user(confirm ? 'Confirm action' : 'Cancel action'),
      );
    });
    _scrollToBottom();

    try {
      final response = await ref
          .read(aiRepositoryProvider)
          .sendMessage(
            message: confirm ? 'confirm' : 'cancel',
            conversationId: _conversationId,
            confirm: confirm,
          );
      setState(() {
        _messages.add(_ChatMessage.assistant(response));
      });
    } catch (error) {
      setState(() {
        _error = error.toString();
      });
    } finally {
      setState(() {
        _sending = false;
      });
      _scrollToBottom();
    }
  }

  void _scrollToBottom() {
    WidgetsBinding.instance.addPostFrameCallback((_) {
      if (!_scrollController.hasClients) {
        return;
      }
      _scrollController.animateTo(
        _scrollController.position.maxScrollExtent,
        duration: const Duration(milliseconds: 220),
        curve: Curves.easeOut,
      );
    });
  }

  @override
  Widget build(BuildContext context) {
    AIChatResponse? lastAssistantResponse;
    for (final message in _messages.reversed) {
      if (message.role == _MessageRole.assistant && message.response != null) {
        lastAssistantResponse = message.response;
        break;
      }
    }
    final needsConfirmation =
        lastAssistantResponse?.response.requiresConfirmation == true;

    return Scaffold(
      appBar: AppBar(title: const Text('Assistant Panel')),
      body: Container(
        decoration: const BoxDecoration(
          gradient: LinearGradient(
            colors: [Color(0xFFF7F4EE), Color(0xFFE6F1F0)],
            begin: Alignment.topLeft,
            end: Alignment.bottomRight,
          ),
        ),
        child: Column(
          children: [
            if (_error != null)
              Container(
                width: double.infinity,
                margin: const EdgeInsets.fromLTRB(12, 10, 12, 6),
                padding: const EdgeInsets.all(10),
                decoration: BoxDecoration(
                  color: const Color(0xFFFFEFEA),
                  borderRadius: BorderRadius.circular(12),
                  border: Border.all(color: const Color(0xFFF0BAA8)),
                ),
                child: Text(
                  _error!,
                  style: const TextStyle(color: Color(0xFF8A3D28)),
                ),
              ),
            Expanded(
              child: _messages.isEmpty
                  ? const _EmptyAssistantState()
                  : ListView.builder(
                      controller: _scrollController,
                      padding: const EdgeInsets.fromLTRB(12, 8, 12, 12),
                      itemCount: _messages.length,
                      itemBuilder: (_, index) {
                        final message = _messages[index];
                        if (message.role == _MessageRole.user) {
                          return _UserBubble(text: message.text);
                        }
                        final response = message.response!;
                        return _AssistantBubble(data: response);
                      },
                    ),
            ),
            if (needsConfirmation)
              _ConfirmationBar(
                onConfirm: _sending ? null : () => _sendConfirmation(true),
                onCancel: _sending ? null : () => _sendConfirmation(false),
              ),
            _Composer(
              controller: _controller,
              sending: _sending,
              onSend: _sendMessage,
            ),
          ],
        ),
      ),
    );
  }
}

class _Composer extends StatelessWidget {
  final TextEditingController controller;
  final bool sending;
  final VoidCallback onSend;

  const _Composer({
    required this.controller,
    required this.sending,
    required this.onSend,
  });

  @override
  Widget build(BuildContext context) {
    return Container(
      padding: const EdgeInsets.fromLTRB(12, 8, 12, 12),
      child: Row(
        children: [
          Expanded(
            child: TextField(
              controller: controller,
              enabled: !sending,
              minLines: 1,
              maxLines: 4,
              decoration: InputDecoration(
                hintText: 'Ask to control a room or device',
                filled: true,
                fillColor: Colors.white,
                border: OutlineInputBorder(
                  borderRadius: BorderRadius.circular(14),
                  borderSide: const BorderSide(color: Color(0xFFD9E4E3)),
                ),
                enabledBorder: OutlineInputBorder(
                  borderRadius: BorderRadius.circular(14),
                  borderSide: const BorderSide(color: Color(0xFFD9E4E3)),
                ),
              ),
              onSubmitted: (_) => onSend(),
            ),
          ),
          const SizedBox(width: 10),
          FilledButton(
            onPressed: sending ? null : onSend,
            child: sending
                ? const SizedBox(
                    width: 16,
                    height: 16,
                    child: CircularProgressIndicator(strokeWidth: 2),
                  )
                : const Icon(Icons.send),
          ),
        ],
      ),
    );
  }
}

class _ConfirmationBar extends StatelessWidget {
  final VoidCallback? onConfirm;
  final VoidCallback? onCancel;

  const _ConfirmationBar({required this.onConfirm, required this.onCancel});

  @override
  Widget build(BuildContext context) {
    return Container(
      width: double.infinity,
      margin: const EdgeInsets.fromLTRB(12, 6, 12, 6),
      padding: const EdgeInsets.all(10),
      decoration: BoxDecoration(
        color: Colors.white,
        borderRadius: BorderRadius.circular(14),
        border: Border.all(color: const Color(0xFFD9E4E3)),
      ),
      child: Row(
        children: [
          const Expanded(
            child: Text(
              'Confirmation required for this action.',
              style: TextStyle(fontWeight: FontWeight.w600),
            ),
          ),
          OutlinedButton(onPressed: onCancel, child: const Text('Cancel')),
          const SizedBox(width: 8),
          FilledButton(onPressed: onConfirm, child: const Text('Confirm')),
        ],
      ),
    );
  }
}

class _EmptyAssistantState extends StatelessWidget {
  const _EmptyAssistantState();

  @override
  Widget build(BuildContext context) {
    return Center(
      child: Container(
        margin: const EdgeInsets.all(18),
        padding: const EdgeInsets.all(18),
        decoration: BoxDecoration(
          color: Colors.white,
          borderRadius: BorderRadius.circular(18),
          border: Border.all(color: const Color(0xFFD9E4E3)),
        ),
        child: const Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            Icon(Icons.smart_toy_outlined, size: 36, color: Color(0xFF0F7B7A)),
            SizedBox(height: 10),
            Text(
              'Structured Assistant Ready',
              style: TextStyle(fontSize: 18, fontWeight: FontWeight.w700),
            ),
            SizedBox(height: 6),
            Text(
              'Try: "Turn off living room" or "Set bedroom lamp to 600"',
              textAlign: TextAlign.center,
            ),
          ],
        ),
      ),
    );
  }
}

class _UserBubble extends StatelessWidget {
  final String text;

  const _UserBubble({required this.text});

  @override
  Widget build(BuildContext context) {
    return Align(
      alignment: Alignment.centerRight,
      child: Container(
        margin: const EdgeInsets.only(bottom: 10),
        padding: const EdgeInsets.symmetric(horizontal: 14, vertical: 10),
        decoration: BoxDecoration(
          color: const Color(0xFF0F7B7A),
          borderRadius: BorderRadius.circular(14),
        ),
        child: Text(text, style: const TextStyle(color: Colors.white)),
      ),
    );
  }
}

class _AssistantBubble extends StatelessWidget {
  final AIChatResponse data;

  const _AssistantBubble({required this.data});

  @override
  Widget build(BuildContext context) {
    final response = data.response;
    final entities = response.entities;
    final plan = response.plan;

    return Align(
      alignment: Alignment.centerLeft,
      child: Container(
        width: double.infinity,
        margin: const EdgeInsets.only(bottom: 10),
        padding: const EdgeInsets.all(12),
        decoration: BoxDecoration(
          color: Colors.white,
          borderRadius: BorderRadius.circular(14),
          border: Border.all(color: const Color(0xFFD9E4E3)),
        ),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              response.reply,
              style: const TextStyle(fontWeight: FontWeight.w600),
            ),
            const SizedBox(height: 8),
            _StructuredLine(label: 'Status', value: response.status),
            _StructuredLine(
              label: 'Intent',
              value:
                  '${response.intent.action} (${response.intent.confidence.toStringAsFixed(2)})',
            ),
            _StructuredLine(
              label: 'Room',
              value: entities.room == null
                  ? '-'
                  : '${entities.room!.name} [${entities.room!.id}]',
            ),
            _StructuredLine(
              label: 'Devices',
              value: entities.devices.isEmpty
                  ? '-'
                  : entities.devices.map((d) => d.name).join(', '),
            ),
            if (plan != null)
              _StructuredLine(
                label: 'Plan',
                value:
                    '${plan.action} / ${plan.comp} / value=${plan.value ?? '-'}',
              ),
            if (response.result != null &&
                response.result!.reasonCodes.isNotEmpty)
              _StructuredLine(
                label: 'Reasons',
                value: response.result!.reasonCodes.join(', '),
              ),
            if (response.result != null && response.result!.items.isNotEmpty)
              _StructuredLine(
                label: 'Tool Items',
                value: response.result!.items
                    .map((item) => '${item.device.name}:${item.status}')
                    .join(', '),
              ),
          ],
        ),
      ),
    );
  }
}

class _StructuredLine extends StatelessWidget {
  final String label;
  final String value;

  const _StructuredLine({required this.label, required this.value});

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.only(bottom: 4),
      child: Text.rich(
        TextSpan(
          text: '$label: ',
          style: const TextStyle(
            fontWeight: FontWeight.w700,
            color: Color(0xFF2B3B3A),
          ),
          children: [
            TextSpan(
              text: value,
              style: const TextStyle(fontWeight: FontWeight.w500),
            ),
          ],
        ),
      ),
    );
  }
}

enum _MessageRole { user, assistant }

class _ChatMessage {
  final _MessageRole role;
  final String text;
  final AIChatResponse? response;

  const _ChatMessage({
    required this.role,
    required this.text,
    required this.response,
  });

  factory _ChatMessage.user(String text) {
    return _ChatMessage(role: _MessageRole.user, text: text, response: null);
  }

  factory _ChatMessage.assistant(AIChatResponse response) {
    return _ChatMessage(
      role: _MessageRole.assistant,
      text: response.response.reply,
      response: response,
    );
  }
}
