"""
Netgroup tests.

:requirement: netgroup
"""

from __future__ import annotations

import pytest
from sssd_test_framework.roles.client import Client
from sssd_test_framework.roles.generic import GenericProvider
from sssd_test_framework.topology import KnownTopologyGroup


@pytest.mark.tier(1)
@pytest.mark.ticket(gh=6652, bz=2162552)
@pytest.mark.topology(KnownTopologyGroup.AnyProvider)
def test_netgroups__add_remove_netgroup_triple(client: Client, provider: GenericProvider):
    """
    :title: Netgroup triple is correctly removed from cached record
    :setup:
        1. Create local user "user-1"
        2. Create netgroup "ng-1"
        3. Add "(-,user-1,)" triple to the netgroup
        4. Start SSSD
    :steps:
        1. Run "getent netgroup ng-1"
        2. Remove "(-,user-1,)" triple from "ng-1"
        3. Invalidate netgroup in cache "sssctl cache-expire -n ng-1"
        4. Run "getent netgroup ng-1"
    :expectedresults:
        1. "(-,user-1,)" is present in the netgroup
        2. Triple was removed from the netgroup
        3. Cached record was invalidated
        4. "(-,user-1,)" is not present in the netgroup
    :customerscenario: True
    """
    user = provider.user("user-1").add()
    ng = provider.netgroup("ng-1").add().add_member(user=user)

    client.sssd.start()

    result = client.tools.getent.netgroup("ng-1")
    assert result is not None
    assert result.name == "ng-1"
    assert len(result.members) == 1
    assert "(-, user-1)" in result.members

    ng.remove_member(user=user)
    client.sssctl.cache_expire(netgroups=True)

    result = client.tools.getent.netgroup("ng-1")
    assert result is not None
    assert result.name == "ng-1"
    assert len(result.members) == 0


@pytest.mark.tier(1)
@pytest.mark.ticket(gh=6652, bz=2162552)
@pytest.mark.topology(KnownTopologyGroup.AnyProvider)
def test_netgroups__add_remove_netgroup_member(client: Client, provider: GenericProvider):
    """
    :title: Netgroup member is correctly removed from cached record
    :setup:
        1. Create local user "user-1"
        2. Create local user "user-2"
        3. Create netgroup "ng-1"
        4. Create netgroup "ng-2"
        5. Add "(-,user-1,)" triple to the netgroup "ng-1"
        6. Add "(-,user-2,)" triple to the netgroup "ng-2"
        7. Add "ng-1" as a member to "ng-2"
        8. Start SSSD
    :steps:
        1. Run "getent netgroup ng-2"
        2. Remove "ng-1" from "ng-2"
        3. Invalidate netgroup "ng-2" in cache "sssctl cache-expire -n ng-2"
        4. Run "getent netgroup ng-2"
    :expectedresults:
        1. "(-,user-1,)", "(-,user-2,)" is present in the netgroup
        2. Netgroup member was removed from the netgroup
        3. Cached record was invalidated
        4. "(-,user-1,)" is not present in the netgroup, only "(-,user-2,)"
    :customerscenario: True
    """
    u1 = provider.user("user-1").add()
    u2 = provider.user("user-2").add()

    ng1 = provider.netgroup("ng-1").add().add_member(user=u1)
    ng2 = provider.netgroup("ng-2").add().add_member(user=u2, ng=ng1)

    client.sssd.start()

    result = client.tools.getent.netgroup("ng-2")
    assert result is not None
    assert result.name == "ng-2"
    assert len(result.members) == 2
    assert "(-, user-1)" in result.members
    assert "(-, user-2)" in result.members

    ng2.remove_member(ng=ng1)
    client.sssctl.cache_expire(netgroups=True)

    result = client.tools.getent.netgroup("ng-2")
    assert result is not None
    assert result.name == "ng-2"
    assert len(result.members) == 1
    assert "(-, user-1)" not in result.members
    assert "(-, user-2)" in result.members

#TODO: change file
@pytest.mark.tier(1)
@pytest.mark.ticket(bz=1921315)
@pytest.mark.topology(KnownTopologyGroup.AnyProvider)
def test_netgroups__auto_private_groups_override(client: Client, provider: GenericProvider):
    """
    :title: Unexpected interaction between overriding the primary group and the
            'auto_private_groups = true' option
    :setup:
        1. Create user "tuser"
        2. Create group "tgroup1" and make "tuser" member of this group
        3. Create group "tgroup2"
        4. Set "auto_private_groups" to true
        5. Start SSSD
        6. Override "tuser" group to "tgroup2"
        7. Restart SSSD
    :steps:
        1. Run "getent group tgroup2"
        2. Run "getent group tgroup1"
    :expectedresults:
        1. "tuser" is a member of "tgroup2"
        2. "tgroup1" member list is empty
    :customerscenario: True
    """
    u = provider.user("tuser").add(uid = 10001, gid = 10001)
    provider.group("tgroup1").add(gid = 20001).add_member(u)
    provider.group("tgroup2").add(gid = 20002)

    client.sssd.domain["auto_private_groups"] = "true"
    client.sssd.start()
    client.host.ssh.run("sss_override user-add tuser -g 20002", raise_on_error=False)
    client.sssd.restart()

    result = client.tools.getent.group("tgroup2")
    assert result is not None
    assert result.name == "tgroup2"
    assert result.gid == 20002
    assert len(result.members) == 1
    assert result.members == ["tuser"]

    print(result)
    result = client.tools.getent.group("tgroup1")
    print(result)
    assert result is not None
    assert result.name == "tgroup1"
    assert result.gid == 20001
    assert len(result.members) == 0
