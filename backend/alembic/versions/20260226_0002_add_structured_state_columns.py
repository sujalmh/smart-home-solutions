"""add structured state columns for ai memory

Revision ID: 20260226_0002
Revises: 20260226_0001
Create Date: 2026-02-26 00:30:00.000000

"""

from typing import Sequence

from alembic import op
import sqlalchemy as sa


revision: str = "20260226_0002"
down_revision: str | None = "20260226_0001"
branch_labels: str | Sequence[str] | None = None
depends_on: str | Sequence[str] | None = None


def _has_column(table_name: str, column_name: str) -> bool:
    bind = op.get_bind()
    inspector = sa.inspect(bind)
    columns = inspector.get_columns(table_name)
    return any(column["name"] == column_name for column in columns)


def upgrade() -> None:
    bind = op.get_bind()
    inspector = sa.inspect(bind)
    tables = set(inspector.get_table_names())
    if "ai_conversations" not in tables:
        return

    if not _has_column("ai_conversations", "pending_plan"):
        op.add_column("ai_conversations", sa.Column("pending_plan", sa.JSON(), nullable=True))
    if not _has_column("ai_conversations", "structured_state"):
        op.add_column("ai_conversations", sa.Column("structured_state", sa.JSON(), nullable=True))


def downgrade() -> None:
    bind = op.get_bind()
    inspector = sa.inspect(bind)
    tables = set(inspector.get_table_names())
    if "ai_conversations" not in tables:
        return

    if _has_column("ai_conversations", "structured_state"):
        op.drop_column("ai_conversations", "structured_state")
    if _has_column("ai_conversations", "pending_plan"):
        op.drop_column("ai_conversations", "pending_plan")
