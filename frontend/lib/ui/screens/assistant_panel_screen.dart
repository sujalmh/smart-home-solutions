import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import '../../config/app_colors.dart';
import '../../config/app_decorations.dart';
import '../../models/ai_chat.dart';
import '../../state/chat_controller.dart';
import '../../state/providers.dart';

class AssistantPanelScreen extends ConsumerStatefulWidget {
  const AssistantPanelScreen({super.key});

  @override
  ConsumerState<AssistantPanelScreen> createState() =>
      _AssistantPanelScreenState();
}

class _AssistantPanelScreenState extends ConsumerState<AssistantPanelScreen> {
  final TextEditingController _controller = TextEditingController();
  final ScrollController _scrollController = ScrollController();

  @override
  void dispose() {
    _controller.dispose();
    _scrollController.dispose();
    super.dispose();
  }

  void _scrollToBottom() {
    WidgetsBinding.instance.addPostFrameCallback((_) {
      if (!_scrollController.hasClients) return;
      _scrollController.animateTo(
        _scrollController.position.maxScrollExtent,
        duration: const Duration(milliseconds: 220),
        curve: Curves.easeOut,
      );
    });
  }

  Future<void> _sendMessage() async {
    final text = _controller.text.trim();
    if (text.isEmpty) return;
    _controller.clear();
    await ref.read(chatControllerProvider.notifier).sendMessage(text);
    _scrollToBottom();
  }

  Future<void> _sendConfirmation(bool confirm) async {
    await ref.read(chatControllerProvider.notifier).sendConfirmation(confirm);
    _scrollToBottom();
  }

  @override
  Widget build(BuildContext context) {
    final chatState = ref.watch(chatControllerProvider);
    final c = context.colors;

    return Scaffold(
      appBar: AppBar(title: const Text('Assistant Panel')),
      body: Container(
        decoration: BoxDecoration(
          gradient: AppDecorations.backgroundGradient(c),
        ),
        child: Column(
          children: [
            if (chatState.error != null)
              Container(
                width: double.infinity,
                margin: const EdgeInsets.fromLTRB(12, 10, 12, 6),
                padding: const EdgeInsets.all(10),
                decoration: BoxDecoration(
                  color: c.errorBg,
                  borderRadius: BorderRadius.circular(kChipRadius),
                  border: Border.all(color: c.errorBorder),
                ),
                child: Text(chatState.error!, style: TextStyle(color: c.error)),
              ),
            Expanded(
              child: chatState.messages.isEmpty
                  ? const _EmptyAssistantState()
                  : ListView.builder(
                      controller: _scrollController,
                      padding: const EdgeInsets.fromLTRB(12, 8, 12, 12),
                      itemCount: chatState.messages.length,
                      itemBuilder: (_, index) {
                        final message = chatState.messages[index];
                        if (message.role == ChatRole.user) {
                          return _UserBubble(text: message.text);
                        }
                        return _AssistantBubble(data: message.response!);
                      },
                    ),
            ),
            if (chatState.needsConfirmation)
              _ConfirmationBar(
                onConfirm: chatState.sending
                    ? null
                    : () => _sendConfirmation(true),
                onCancel: chatState.sending
                    ? null
                    : () => _sendConfirmation(false),
              ),
            _Composer(
              controller: _controller,
              sending: chatState.sending,
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
                hintText: 'Ask me anything...',
                filled: true,
                fillColor: Colors.white,
                border: OutlineInputBorder(
                  borderRadius: BorderRadius.circular(kButtonRadius),
                  borderSide: BorderSide(color: context.colors.composerBorder),
                ),
                enabledBorder: OutlineInputBorder(
                  borderRadius: BorderRadius.circular(kButtonRadius),
                  borderSide: BorderSide(color: context.colors.composerBorder),
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
        borderRadius: BorderRadius.circular(kButtonRadius),
        border: Border.all(color: context.colors.composerBorder),
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
    final c = context.colors;
    return Center(
      child: Container(
        margin: const EdgeInsets.all(18),
        padding: const EdgeInsets.all(18),
        decoration: BoxDecoration(
          color: Colors.white,
          borderRadius: BorderRadius.circular(kCardRadius),
          border: Border.all(color: c.composerBorder),
        ),
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            Icon(Icons.smart_toy_outlined, size: 36, color: c.primary),
            SizedBox(height: 10),
            Text(
              'Smart Home Assistant',
              style: TextStyle(fontSize: 18, fontWeight: FontWeight.w700),
            ),
            SizedBox(height: 6),
            Text(
              'Ask me anything \u2014 control devices, check status, or just chat!',
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
          color: context.colors.primary,
          borderRadius: BorderRadius.circular(kButtonRadius),
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
    final hasToolItems =
        response.result != null && response.result!.items.isNotEmpty;
    final screenWidth = MediaQuery.of(context).size.width;

    return Align(
      alignment: Alignment.centerLeft,
      child: ConstrainedBox(
        constraints: BoxConstraints(maxWidth: screenWidth * 0.85),
        child: Container(
          margin: const EdgeInsets.only(bottom: 10),
          padding: const EdgeInsets.all(12),
          decoration: BoxDecoration(
            color: Colors.white,
            borderRadius: BorderRadius.circular(kButtonRadius),
            border: Border.all(color: context.colors.composerBorder),
          ),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              SelectableText(
                response.reply,
                style: const TextStyle(fontSize: 14, height: 1.4),
              ),
              if (hasToolItems) ...[
                const SizedBox(height: 8),
                Wrap(
                  spacing: 6,
                  runSpacing: 4,
                  children: response.result!.items.map((item) {
                    final ok = item.status == 'ok';
                    return Chip(
                      avatar: Icon(
                        ok ? Icons.check_circle : Icons.error,
                        size: 16,
                        color: ok ? Colors.green : Colors.red,
                      ),
                      label: Text(
                        item.device.name,
                        style: const TextStyle(fontSize: 12),
                      ),
                      visualDensity: VisualDensity.compact,
                    );
                  }).toList(),
                ),
              ],
            ],
          ),
        ),
      ),
    );
  }
}
