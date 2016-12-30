/*
 * qspi-winbond.h
 *
 * Copyright (c) 2016 Lix N. Paulian (lix@paulian.net)
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Created on: 29 Dec 2016 (LNP)
 *
 * Version: 0.2, 31 Dec 2016
 */

#ifndef QSPI_WINBOND_H_
#define QSPI_WINBOND_H_

#include "qspi-flash.h"


#if defined (__cplusplus)

class qspi_winbond: public qspi_impl
{

public:

  virtual bool
  mode_quad (qspi* pq) override;

  virtual bool
  memory_mapped (qspi* pq) override;

  virtual bool
  read (qspi* pq, uint32_t address, uint8_t* buff, size_t count) override;

  virtual bool
  page_write (qspi* pq, uint32_t address, uint8_t* buff, size_t count) override;

//  bool
//  sector_erase (uint32_t address);
//
//  bool
//  block32K_erase (uint32_t address);
//
//  bool
//  block64K_erase (uint32_t address);
//
//  bool
//  chip_erase (void);

private:
    // Winbond specific commands
  static constexpr uint8_t VOLATILE_SR_WRITE_ENABLE = 0x50;
  static constexpr uint8_t READ_STATUS_REGISTER_2 = 0x35;
  static constexpr uint8_t WRITE_STATUS_REGISTER_2 = 0x31;
  static constexpr uint8_t READ_STATUS_REGISTER_3 = 0x15;
  static constexpr uint8_t WRITE_STATUS_REGISTER_3 = 0x11;


};


#endif

#endif /* QSPI_WINBOND_H_ */