# pdlobbyd is stdlib-only Python, so the image is a interpreter and a file.
FROM python:3.12-slim

# The script never writes anything (rooms live in memory), so it runs as a
# user with nothing to break.
RUN useradd --system --uid 10001 --no-create-home lobby

COPY tools/pdlobbyd/pdlobbyd.py /app/pdlobbyd.py

USER lobby
WORKDIR /app

# Informational only: the compose file uses host networking, because the UDP
# rendezvous must see each player's real source address, which a bridge's
# NAT would erase (every player would look like the docker0 address and no
# punch could ever be aimed at anything).
EXPOSE 8091/tcp 27101/udp

HEALTHCHECK --interval=30s --timeout=5s --start-period=5s \
  CMD python3 -c "import os,urllib.request,sys; \
    p=os.environ.get('PDLOBBYD_PORT','8091'); \
    sys.exit(0 if urllib.request.urlopen('http://127.0.0.1:'+p+'/ping', timeout=4).status==200 else 1)" \
    || exit 1

ENTRYPOINT ["python3", "/app/pdlobbyd.py"]
