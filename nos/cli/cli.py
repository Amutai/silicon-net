"""
silicon-net: Mini-NOS CLI
Simple command interface that writes intent to APP_DB (Redis).

Usage:
    python cli.py

Commands:
    port <id> up|down
    fdb add <mac> vlan <vlan_id> port <port_id>
    fdb del <mac> vlan <vlan_id>
    route add <prefix>/<len> nexthop <ip> port <port_id>
    route del <prefix>/<len>
    acl add src-ip <ip>/<mask> action permit|deny
    show fdb | routes | counters | ports
    inject-packet port <id> dst-mac <mac> src-ip <ip> dst-ip <ip>
"""

# TODO: Implement in Milestone 4

import click
import redis


APP_DB = 0


@click.group()
@click.pass_context
def cli(ctx):
    """silicon-net Mini-NOS CLI"""
    ctx.ensure_object(dict)
    ctx.obj["redis"] = redis.Redis(host="localhost", port=6379, db=APP_DB)


@cli.command()
@click.argument("port_id", type=int)
@click.argument("state", type=click.Choice(["up", "down"]))
@click.pass_context
def port(ctx, port_id: int, state: str):
    """Set port admin state."""
    r = ctx.obj["redis"]
    r.hset(f"PORT_TABLE:port{port_id}", mapping={"admin_status": state})
    click.echo(f"Port {port_id} → {state}")


@cli.group()
def route():
    """Route operations."""
    pass


@route.command("add")
@click.argument("prefix")
@click.option("--nexthop", required=True)
@click.option("--port", "port_id", required=True, type=int)
@click.pass_context
def route_add(ctx, prefix: str, nexthop: str, port_id: int):
    """Add a route. Example: route add 10.0.0.0/24 --nexthop 192.168.1.1 --port 3"""
    r = ctx.obj["redis"]
    r.hset(f"ROUTE_TABLE:{prefix}", mapping={
        "nexthop": nexthop,
        "ifname": f"port{port_id}"
    })
    click.echo(f"Route {prefix} → nhop {nexthop} port {port_id}")


@route.command("del")
@click.argument("prefix")
@click.pass_context
def route_del(ctx, prefix: str):
    """Delete a route."""
    r = ctx.obj["redis"]
    r.delete(f"ROUTE_TABLE:{prefix}")
    click.echo(f"Route {prefix} removed")


@cli.group()
def show():
    """Show commands."""
    pass


@show.command("routes")
@click.pass_context
def show_routes(ctx):
    """Show all routes."""
    r = ctx.obj["redis"]
    for key in r.scan_iter("ROUTE_TABLE:*"):
        data = r.hgetall(key)
        prefix = key.decode().split(":", 1)[1]
        nhop = data.get(b"nexthop", b"?").decode()
        iface = data.get(b"ifname", b"?").decode()
        click.echo(f"  {prefix} → nexthop={nhop} iface={iface}")


if __name__ == "__main__":
    cli()
