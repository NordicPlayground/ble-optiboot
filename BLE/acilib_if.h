/* Copyright (c) 2014, Nordic Semiconductor ASA
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * @file
 *
 * @ingroup group_acilib
 *
 * @brief Prototypes for the acilib interfaces.
 */

#ifndef _acilib_IF_H_
#define _acilib_IF_H_

#include <stdbool.h>

#include "aci_cmds.h"

/** @brief Encode the ACI message to connect
 *
 *  @param[in,out]  buffer                    Pointer to ACI message buffer
 *  @param[in]      p_aci_cmd_params_connect  Pointer to the run parameters in ::aci_cmd_params_connect_t
 *
 *  @return         None
 */
void acil_encode_cmd_connect(uint8_t *buffer, aci_cmd_params_connect_t *p_aci_cmd_params_connect);

/** @brief Encode the ACI message to disconnect
 *
 *  @param[in,out]  buffer                       Pointer to ACI message buffer
 *  @param[in]      p_aci_cmd_params_disconnect  Pointer to the run parameters in ::aci_cmd_params_disconnect_t
 *
 *  @return         None
 */
void acil_encode_cmd_disconnect(uint8_t *buffer, aci_cmd_params_disconnect_t *p_aci_cmd_params_disconnect);

/** @brief Encode the ACI message for send data
 *
 *  @param[in,out]  buffer                        Pointer to ACI message buffer
 *  @param[in]      p_aci_cmd_params_send_data_t  Pointer to the data parameters in ::aci_cmd_params_send_data_t
 *  @param[in]      data_size                     Size of data message
 *
 *  @return         None
 */
void acil_encode_cmd_send_data(uint8_t *buffer, aci_cmd_params_send_data_t *p_aci_cmd_params_send_data_t, uint8_t data_size);

#endif
