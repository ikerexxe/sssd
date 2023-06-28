from __future__ import annotations

import pytest

from sssd_test_framework.roles.client import Client
from sssd_test_framework.roles.generic import GenericProvider
from sssd_test_framework.topology import KnownTopology

@pytest.mark.tier(0)
@pytest.mark.topology(KnownTopology.LDAP)
def test_pam__basic_useradd(client: Client, provider: GenericProvider):
    """
    :title: Add user with password and authenticate
    :setup:
        1. Add user "tuser" with password "Secret123"
    :steps:
        1. ssh as "tuser"
    :expectedresults:
        1. ssh successfully
    :customerscenario: False
    """
    client.local.user("tuser").add(password = "Secret123")

    result = client.auth.ssh.password("tuser", "Secret123")
    assert result is not None

@pytest.mark.tier(0)
@pytest.mark.topology(KnownTopology.LDAP)
def test_pam__useradd_force_uid(client: Client, provider: GenericProvider):
    """
    :title: Add user with password and authenticate
    :setup:
        1. Add user "tuser" with password "Secret123"
    :steps:
        1. id "tuser"
    :expectedresults:
        1. "tuser" exists and id is 2000
    :customerscenario: False
    """
    client.local.user("tuser").add(uid = 2000)

    result = client.tools.id("tuser")
    assert result is not None
    assert result.user.name == "tuser"
    assert result.user.id == 2000
