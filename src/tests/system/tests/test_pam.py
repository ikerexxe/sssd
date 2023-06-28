from __future__ import annotations

import pytest
import re

from sssd_test_framework.roles.client import Client
from sssd_test_framework.roles.generic import GenericProvider
from sssd_test_framework.topology import KnownTopology

@pytest.mark.tier(1)
@pytest.mark.ticket(bz=2091062)
@pytest.mark.topology(KnownTopology.LDAP)
def test_pam__motd_directory_error(client: Client, provider: GenericProvider):
    """
    :title: "error scanning directory" errors from pam_motd
    :setup:
        1. Install and enable "rsyslog"
        2. Add user "tuser" with password "Secret123"
        3. Remove motd.d folders
    :steps:
        1. ssh as "tuser"
        2. Search for pam_motd directory error
    :expectedresults:
        1. ssh successfully
        2. Error shouldn't appear in logs
    :customerscenario: True
    """
    client.host.ssh.exec(["dnf", "install", "-y", "rsyslog"])
    #client.host.services.start("rsyslog") #TODO: try checking journal information
    client.local.user("tuser").add(password = "Secret123")
    for folder in ['/run/motd.d', '/etc/motd.d', '/usr/lib/motd.d']:
        client.fs.backup(folder)

    result = client.auth.ssh.password("tuser", "Secret123")
    assert result is not None

    log = client.fs.read("/var/log/secure")
    result = re.search(r"pam_motd: error scanning directory", log)
    assert result is None

@pytest.mark.tier(1)
@pytest.mark.ticket(bz=2014458)
@pytest.mark.topology(KnownTopology.LDAP)
def test_pam__motd_multiple_paths(client: Client, provider: GenericProvider):
    """
    :title: Please backport checking multiple motd_pam paths from 1.4.0 to RHEL 8
    :setup:
        1. Add user "tuser" with password "Secret123"
        2. Write welcome message to "/etc/motd.d/welcome"
    :steps:
        1. ssh as "tuser"
    :expectedresults:
        1. ssh successfully and welcome message is printed in stdout
    :customerscenario: True
    """
    client.local.user("tuser").add(password = "Secret123")
    client.fs.write("/etc/motd.d/welcome", "Welcome to this system")

    result = client.auth.ssh.password("tuser", "Secret123")
    assert result is not None
    #TODO: assert welcome message is printed in stdout
