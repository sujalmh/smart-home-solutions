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
                hintText: 'Ask to control a room or device',
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
          borderRadius: BorderRadius.circular(kButtonRadius),
          border: Border.all(color: context.colors.composerBorder),
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
          style: TextStyle(
            fontWeight: FontWeight.w700,
            color: context.colors.heading,
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
