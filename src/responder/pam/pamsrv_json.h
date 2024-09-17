/*
    SSSD

    pamsrv_json authentication selection helper for GDM

    Authors:
        Iker Pedrosa <ipedrosa@redhat.com>

    Copyright (C) 2024 Red Hat

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __PAMSRV_JSON__H__
#define __PAMSRV_JSON__H__

#include <jansson.h>
#include <talloc.h>

#include "util/sss_pam_data.h"

struct auth_data {
    struct password_data *pswd;
    struct oauth2_data *oauth2;
    struct sc_data *sc;
    struct passkey_data *passkey;
};

struct password_data {
    bool enabled;
    char *prompt;
};

struct oauth2_data {
    bool enabled;
    char *uri;
    char *code;
    char *init_prompt;
    char *link_prompt;
};

struct sc_data {
    bool enabled;
    char **names;
    char *init_prompt;
    char *pin_prompt;
};

struct passkey_data {
    bool enabled;
    char *init_prompt;
    bool pin_request;
    int pin_attempts;
    char *pin_prompt;
    char *touch_prompt;
};


/**
 * @brief Extract smartcard certificate list from pam_data structure
 *
 * @param[in] mem_ctx Memory context
 * @param[in] pd pam_data containing the certificates
 * @param[out] _cert_list Certificate list
 *
 * @return 0 if the data was extracted successfully,
 *         error code otherwise.
 */
errno_t
get_cert_list(TALLOC_CTX *mem_ctx, struct pam_data *pd,
              struct cert_auth_info **_cert_list);

/**
 * @brief Extract smartcard certificate name list from the certificate list
 *
 * @param[in] mem_ctx Memory context
 * @param[in] cert_list Certificate list
 * @param[out] _auth_data Structure containing the data from all available
 *             authentication mechanisms
 *
 * @return 0 if the data was extracted successfully,
 *         error code otherwise.
 */
errno_t
get_cert_names(TALLOC_CTX *mem_ctx, struct cert_auth_info *cert_list,
               struct auth_data *_auth_data);

/**
 * @brief Format authentication mechanisms to JSON
 *
 * @param[in] auth_data Structure containing the data from all available
 *            authentication mechanisms
 * @param[out] _list_mech authentication mechanisms JSON object
 *
 * @return 0 if the authentication mechanisms were formatted properly,
 *         error code otherwise.
 */
errno_t
json_format_mechanisms(struct auth_data *auth_data, json_t **_list_mech);

/**
 * @brief Format priority to JSON
 *
 * @param[in] auth_data Structure containing the data from all available
 *            authentication mechanisms
 * @param[out] _priority priority JSON object
 *
 * @return 0 if the priority was formatted properly,
 *         error code otherwise.
 */
errno_t
json_format_priority(struct auth_data *auth_data, json_t **_priority);

/**
 * @brief Format data to JSON
 *
 * @param[in] mem_ctx Memory context
 * @param[in] auth_data Structure containing the data from all available
 *            authentication mechanisms
 * @param[out] _result JSON message
 *
 * @return 0 if the JSON message was formatted properly,
 *         error code otherwise.
 */
errno_t
json_format_auth_selection(TALLOC_CTX *mem_ctx, struct auth_data *auth_data,
                           char **_result);

/**
 * @brief Check the internal data and generate the JSON message
 *
 * @param[in] cdb The connection object to the confdb
 * @param[in] pc_list List that contains all authentication mechanisms prompts
 * @param[out] pd Data structure containing the response_data linked list
 *
 * @return 0 if the data was extracted correctly and JSON message was formatted
 *         properly, error code otherwise.
 */
errno_t
generate_json_auth_message(struct confdb_ctx *cdb,
                           struct prompt_config **pc_list,
                           struct pam_data *_pd);


/**
 * @brief Unpack password specific data reply
 *
 * @param[in] jroot jansson structure containing the password specific data
 * @param[out] _password user password
 *
 * @return 0 if the reply was unpacked and the result is ok,
 *         error code otherwise.
 */
errno_t
json_unpack_password(json_t *jroot, char **_password);

/**
 * @brief Unpack OAUTH2 code
 *
 * @param[in] mem_ctx Memory context
 * @param[in] json_auth_msg JSON authentication mechanisms message
 * @param[out] _oauth2_code OAUTH2 code
 *
 * @return 0 if the reply was unpacked and the result is ok,
 *         error code otherwise.
 */
errno_t
json_unpack_oauth2_code(TALLOC_CTX *mem_ctx, char *json_auth_msg,
                        char **_oauth2_code);

/**
 * @brief Unpack data reply containing PIN
 *
 * @param[in] jroot jansson structure containing the data
 * @param[out] _pin user PIN
 */
errno_t
json_unpack_pin(json_t *jroot, char **_pin);

/**
 * @brief Unpack GDM reply and check its value
 *
 * @param[in] pd pam_data containing the GDM reply in JSON format
 *
 * @return 0 if the reply was unpacked and the result is ok,
 *         error code otherwise.
 */
errno_t
json_unpack_auth_reply(struct pam_data *pd);

/**
 * @brief Check whether the PAM service file in use is enabled for the JSON
 * protocol
 *
 * @param[in] json_services Enabled PAM services for JSON protocol
 * @param[in] service PAM service file in use
 *
 * @return true if the JSON protocol is enabled for the PAM service file,
 *         false otherwise.
 */
bool is_pam_json_enabled(char **json_services,
                         char *service);

#endif /* __PAMSRV_JSON__H__ */
