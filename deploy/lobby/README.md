# The lobby, in Docker

`pdlobbyd` (see `tools/pdlobbyd/README.md` for what the lobby *is*) behind
Caddy for TLS, on one machine with a domain pointing at it.

```sh
cp .env.example .env      # set LOBBY_DOMAIN
docker compose up -d --build
curl -s https://$LOBBY_DOMAIN/pdlobby/ping
```

Then tell players to install the fork build and set their lobby server
(Options, or `pd.ini`) to:

```
https://<your domain>/pdlobby
```

## Why `network_mode: host`

Two reasons, both load-bearing:

- The UDP rendezvous answers a client's REGISTER and ECHO with *the address
  it saw the packet come from*, and the NAT punches are aimed at exactly
  that address. A compose bridge NATs every source to its gateway, so every
  player would appear as `172.17.0.1` and no pair would ever connect.
- Let's Encrypt's TLS-ALPN challenge wants :443, which is simpler to give
  Caddy on the host network than to route through published ports.

The lobby's own HTTP port binds loopback only; Caddy is the only thing that
talks to it, and the only ports open to the world are Caddy's :443 and the
lobby's UDP.

## Firewall

| | |
| --- | --- |
| TCP 443 | Caddy: HTTPS and ACME |
| UDP 27101 | the rendezvous (`LOBBY_UDP_PORT`) |
| UDP 27110-27141 | the relays (`LOBBY_RELAY_PORTS`), one per pair that cannot punch |

Players' own game ports (default UDP 27005, `Net.Port`) are *theirs*; the
lobby never routes a match, only pairs that failed to punch touch the relay
range.

## Auth

`LOBBY_AUTH=open` (the default here) lets anyone play under any name - fine
for a lobby whose address you only give to people you would give a LAN
password to. `LOBBY_AUTH=ghost` makes every create and join check a name and
PIN against a `pdghostd` (`tools/pdghostd/`); that needs its own container,
its `ghosts.db` volume, and `PDLOBBYD_GHOST_URL` pointed at it - not built
here yet because Ghost Trials and lobby accounts are a second decision.

## Operations

Rooms are in memory: a restart empties the lobby and every host re-creates
its room (the client says so politely). `docker stop` sends SIGTERM, and the
lobby tells every room before closing it (`PDLOBBYD_SHUTDOWN_NOTICE` says
what it says). Check on it with:

```sh
docker compose logs -f lobby     # rooms made, launched, closed
docker inspect --format '{{.State.Health.Status}}' $(docker compose ps -q lobby)
```

## Tested

`deploy/lobby` was checked by running the game's own lobby loop against a
container started exactly as the compose file starts it (host network,
UDP 27101): three clients - host, member on a team, spectator - over HTTP on
loopback with the UDP rendezvous inside the container (`lobby echo 0 ms` in
the client's Briefing Room line, room made, launched, closed in its log).
What only a real machine can test is the TLS leg and punch behaviour from
different NATs behind the public address.
