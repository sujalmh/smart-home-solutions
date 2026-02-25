"""add ai conversation and audit tables

Revision ID: 20260226_0001
Revises:
Create Date: 2026-02-26 00:00:00.000000

"""

from typing import Sequence

from alembic import op
import sqlalchemy as sa


# revision identifiers, used by Alembic.
revision: str = "20260226_0001"
down_revision: str | None = None
branch_labels: str | Sequence[str] | None = None
depends_on: str | Sequence[str] | None = None


def upgrade() -> None:
    bind = op.get_bind()
    inspector = sa.inspect(bind)
    tables = set(inspector.get_table_names())

    if "ai_conversations" not in tables:
        op.create_table(
            "ai_conversations",
            sa.Column("conversation_id", sa.String(length=64), primary_key=True),
            sa.Column("is_active", sa.Boolean(), nullable=False, server_default=sa.true()),
            sa.Column("last_room_id", sa.String(length=255), nullable=True),
            sa.Column("last_device_id", sa.String(length=255), nullable=True),
            sa.Column("last_intent", sa.String(length=64), nullable=True),
            sa.Column("pending_confirmation", sa.JSON(), nullable=True),
            sa.Column("expires_at", sa.DateTime(timezone=True), nullable=True),
            sa.Column("created_at", sa.DateTime(timezone=True), nullable=False),
            sa.Column("updated_at", sa.DateTime(timezone=True), nullable=False),
        )
        op.create_index(
            "ix_ai_conversations_is_active",
            "ai_conversations",
            ["is_active"],
            unique=False,
        )

    tables = set(inspector.get_table_names())
    if "ai_audit_logs" not in tables:
        op.create_table(
            "ai_audit_logs",
            sa.Column("id", sa.Integer(), primary_key=True, autoincrement=True),
            sa.Column("conversation_id", sa.String(length=64), nullable=False),
            sa.Column("utterance", sa.Text(), nullable=False),
            sa.Column("intent", sa.String(length=64), nullable=True),
            sa.Column("entities", sa.JSON(), nullable=True),
            sa.Column("confidence", sa.Float(), nullable=True),
            sa.Column("safety_decision", sa.String(length=32), nullable=False),
            sa.Column("selected_action", sa.String(length=128), nullable=True),
            sa.Column("result", sa.Text(), nullable=True),
            sa.Column("response_text", sa.Text(), nullable=True),
            sa.Column("created_at", sa.DateTime(timezone=True), nullable=False),
            sa.ForeignKeyConstraint(
                ["conversation_id"],
                ["ai_conversations.conversation_id"],
                ondelete="CASCADE",
            ),
        )
        op.create_index(
            "ix_ai_audit_logs_conversation_id",
            "ai_audit_logs",
            ["conversation_id"],
            unique=False,
        )
        op.create_index(
            "ix_ai_audit_logs_created_at",
            "ai_audit_logs",
            ["created_at"],
            unique=False,
        )


def downgrade() -> None:
    bind = op.get_bind()
    inspector = sa.inspect(bind)
    tables = set(inspector.get_table_names())

    if "ai_audit_logs" in tables:
        op.drop_index("ix_ai_audit_logs_created_at", table_name="ai_audit_logs")
        op.drop_index("ix_ai_audit_logs_conversation_id", table_name="ai_audit_logs")
        op.drop_table("ai_audit_logs")

    tables = set(inspector.get_table_names())
    if "ai_conversations" in tables:
        op.drop_index("ix_ai_conversations_is_active", table_name="ai_conversations")
        op.drop_table("ai_conversations")
