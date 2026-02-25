"""make user device id optional

Revision ID: 20260226_0003
Revises: 20260226_0002
Create Date: 2026-02-26 01:10:00.000000

"""

from typing import Sequence

from alembic import op
import sqlalchemy as sa


revision: str = "20260226_0003"
down_revision: str | None = "20260226_0002"
branch_labels: str | Sequence[str] | None = None
depends_on: str | Sequence[str] | None = None


def _has_table(table_name: str) -> bool:
    bind = op.get_bind()
    inspector = sa.inspect(bind)
    return table_name in set(inspector.get_table_names())


def _has_column(table_name: str, column_name: str) -> bool:
    bind = op.get_bind()
    inspector = sa.inspect(bind)
    columns = inspector.get_columns(table_name)
    return any(column["name"] == column_name for column in columns)


def upgrade() -> None:
    if not _has_table("users") or not _has_column("users", "device_id"):
        return

    with op.batch_alter_table("users") as batch_op:
        batch_op.alter_column("device_id", existing_type=sa.String(length=255), nullable=True)


def downgrade() -> None:
    if not _has_table("users") or not _has_column("users", "device_id"):
        return

    with op.batch_alter_table("users") as batch_op:
        batch_op.alter_column(
            "device_id",
            existing_type=sa.String(length=255),
            nullable=False,
            server_default="",
        )
