"""
silicon-net: Mini-NOS CLI
Command interface that writes intent to APP_DB (Redis).

Usage:
    python -m nos.cli.cli
"""

import click
import redis

from nos.schema import APP_DB, port_table_key, route_table_key, fdb_table_key


@click.group()
@click.pass_context
def cli(ctx):
    """silicon-net Mini-NOS CLI"""
    ctx.ensure_object(dict)
    ctx.obj["redis"] = redis.Redis(host="localhost", port=6379, db=APP_DB, decode_responses=True)


# --- Port Commands (#36) ---

@cli.command()
@click.argument("port_id", type=int)
@click.argument("state", type=click.Choice(["up", "down"]))
@click.pass_context
def port(ctx, port_id: int, state: str):
    """Set port admin state. Example: port 0 up"""
    r = ctx.obj["redis"]
    r.hset(port_table_key(port_id), mapping={"admin_status": state})
    click.echo(f"Port {port_id} → {state}")


# --- FDB Commands (#36) ---

@cli.group()
def fdb():
    """FDB operations."""
    pass


@fdb.command("add")
@click.argument("mac")
@click.option("--vlan", required=True, type=int)
@click.option("--port", "port_id", required=True, type=int)
@click.pass_context
def fdb_add(ctx, mac: str, vlan: int, port_id: int):
    """Add FDB entry. Example: fdb add 00:11:22:33:44:55 --vlan 1 --port 2"""
    r = ctx.obj["redis"]
    r.hset(fdb_table_key(vlan, mac), mapping={
        "port": f"port{port_id}",
        "type": "static",
    })
    click.echo(f"FDB: {mac} vlan {vlan} → port {port_id}")


@fdb.command("del")
@click.argument("mac")
@click.option("--vlan", required=True, type=int)
@click.pass_context
def fdb_del(ctx, mac: str, vlan: int):
    """Delete FDB entry."""
    r = ctx.obj["redis"]
    r.delete(fdb_table_key(vlan, mac))
    click.echo(f"FDB: {mac} vlan {vlan} removed")


# --- Route Commands (#36) ---

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
    """Add route. Example: route add 10.0.0.0/24 --nexthop 192.168.1.1 --port 3"""
    r = ctx.obj["redis"]
    r.hset(route_table_key(prefix), mapping={
        "nexthop": nexthop,
        "ifname": f"port{port_id}",
    })
    click.echo(f"Route: {prefix} → nexthop {nexthop} port {port_id}")


@route.command("del")
@click.argument("prefix")
@click.pass_context
def route_del(ctx, prefix: str):
    """Delete route."""
    r = ctx.obj["redis"]
    r.delete(route_table_key(prefix))
    click.echo(f"Route: {prefix} removed")


# --- Show Commands (#37) ---

@cli.group()
def show():
    """Show commands."""
    pass


@show.command("ports")
@click.pass_context
def show_ports(ctx):
    """Show port states."""
    r = ctx.obj["redis"]
    for key in sorted(r.scan_iter("PORT_TABLE:*")):
        data = r.hgetall(key)
        name = key.split(":")[1]
        status = data.get("admin_status", "unknown")
        click.echo(f"  {name}: {status}")


@show.command("routes")
@click.pass_context
def show_routes(ctx):
    """Show all routes."""
    r = ctx.obj["redis"]
    for key in sorted(r.scan_iter("ROUTE_TABLE:*")):
        data = r.hgetall(key)
        prefix = key.split(":", 1)[1]
        nhop = data.get("nexthop", "?")
        iface = data.get("ifname", "?")
        click.echo(f"  {prefix} → nexthop={nhop} iface={iface}")


@show.command("fdb")
@click.pass_context
def show_fdb(ctx):
    """Show FDB table."""
    r = ctx.obj["redis"]
    for key in sorted(r.scan_iter("FDB_TABLE:*")):
        data = r.hgetall(key)
        parts = key.split(":")
        vlan = parts[1]
        mac = parts[2]
        port_name = data.get("port", "?")
        entry_type = data.get("type", "?")
        click.echo(f"  {mac} {vlan} → {port_name} ({entry_type})")


# --- Packet Injection (#38) ---

@cli.command("inject-packet")
@click.option("--port", "port_id", required=True, type=int)
@click.option("--dst-mac", default="00:00:00:00:00:00")
@click.option("--src-ip", default="0.0.0.0")
@click.option("--dst-ip", required=True)
@click.option("--ttl", default=64, type=int)
@click.pass_context
def inject_packet(ctx, port_id: int, dst_mac: str, src_ip: str, dst_ip: str, ttl: int):
    """Inject a packet for testing. Example: inject-packet --port 0 --dst-ip 10.0.2.5"""
    click.echo(f"Injecting packet: port={port_id} dst_mac={dst_mac} "
               f"src_ip={src_ip} dst_ip={dst_ip} ttl={ttl}")
    click.echo("(Packet injection requires SAI ctypes binding — see syncd-lite)")


if __name__ == "__main__":
    cli()
