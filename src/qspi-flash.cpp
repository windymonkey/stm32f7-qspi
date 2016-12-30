/*
 * qspi-flash.cpp
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
 * Created on: 9 Oct 2016 (LNP)
 *
 * Version: 0.2, 31 Dec 2016
 */

/*
 * This file implements the basic low level functions to control
 * a QSPI flash device.
 */

#include <cmsis-plus/rtos/os.h>
#include <cmsis-plus/diag/trace.h>
#include "qspi-flash.h"

#include "qspi-winbond.h"

using namespace os;


qspi::qspi (QSPI_HandleTypeDef* hqspi)
{
  trace::printf ("%s(%p) @%p\n", __func__, hqspi, this);
  hqspi_ = hqspi;
}

/**
 * @brief  Read the memory parameters (manufacturer, type and capacity).
 * @return true if successful, false otherwise.
 */
bool
qspi::read_JEDEC_ID (void)
{
  bool result = false;
  uint8_t buff[3];
  QSPI_CommandTypeDef sCommand;

  if (mutex_.timed_lock (QSPI_TIMEOUT) == rtos::result::ok)
    {
      // Read command settings
      sCommand.AddressSize = QSPI_ADDRESS_24_BITS;
      sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
      sCommand.DdrMode = QSPI_DDR_MODE_DISABLE;
      sCommand.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
      sCommand.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;
      sCommand.InstructionMode = QSPI_INSTRUCTION_1_LINE;
      sCommand.AddressMode = QSPI_ADDRESS_NONE;
      sCommand.DataMode = QSPI_DATA_1_LINE;
      sCommand.DummyCycles = 0;
      sCommand.NbData = 3;
      sCommand.Instruction = JEDEC_ID;

      HAL_QSPI_Abort (hqspi_);

      // Initiate read and wait for the event
      if (HAL_QSPI_Command (hqspi_, &sCommand, QSPI_TIMEOUT) == HAL_OK)
	{
	  if (HAL_QSPI_Receive_IT (hqspi_, buff) == HAL_OK)
	    {
	      if (semaphore_.timed_wait (QSPI_TIMEOUT) == rtos::result::ok)
		{
		  manufacturer_ID_ = buff[0];
		  memory_type_ = buff[1];
		  memory_capacity_ = buff[2];
		  valid_mem_ID = true;
		  result = true;
		}
	    }
	}
      if (result == false)
	{
	  HAL_QSPI_Abort (hqspi_);
	}
      mutex_.unlock ();
    }
  return result;
}

bool
qspi::get_ID_data (uint8_t& manufacturer_ID, uint8_t& memory_type,
	       uint8_t& memory_capacity)
{
  bool result = false;

  if (valid_mem_ID)
    {
	manufacturer_ID = manufacturer_ID_;
	memory_type = memory_type_;
	memory_capacity = memory_capacity_;
	pimpl = new qspi_winbond {};
	result = true;
    }
  return result;
}

/**
 * @brief  Write data to flash.
 * @param  address: start address in flash where to write data to.
 * @param  buff: source data to be written.
 * @param  count: amount of data to be written.
 * @return true if successful, false otherwise.
 */
bool
qspi::write (uint32_t address, uint8_t* buff, size_t count)
{
  bool result = true;
  size_t in_block_count;

  HAL_QSPI_Abort (hqspi_);

  do
    {
      in_block_count = 0x100 - (address & 0xFF);
      if (in_block_count > count)
	in_block_count = count;
      else if (in_block_count == 0)
	{
	  in_block_count = (count > 0x100) ? 0x100 : count;
	}
      if (page_write (address, buff, in_block_count) == false)
	{
	  result = false;
	  break;
	}
      address += in_block_count;
      buff += in_block_count;
      count -= in_block_count;
    }
  while (count);

  return result;
}

/**
 * @brief  Erase a sector (4K), block (32K), large block (64K) or whole flash.
 * @param  address: address of the block to be erased.
 * @param  which: command to erase, can be either SECTOR_ERASE, BLOCK_32K_ERASE,
 * 	BLOCK_64K_ERASE or CHIP_ERASE.
 * @return true if successful, false otherwise.
 */
bool
qspi::erase (uint32_t address, uint8_t which)
{
  bool result = false;
  QSPI_CommandTypeDef sCommand;
  QSPI_AutoPollingTypeDef sConfig;

  if (mutex_.timed_lock (QSPI_TIMEOUT) == rtos::result::ok)
    {
      // Initial command settings
      sCommand.AddressSize = QSPI_ADDRESS_24_BITS;
      sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
      sCommand.DdrMode = QSPI_DDR_MODE_DISABLE;
      sCommand.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
      sCommand.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;
      sCommand.InstructionMode = QSPI_INSTRUCTION_1_LINE;
      sCommand.AddressMode = QSPI_ADDRESS_NONE;
      sCommand.DataMode = QSPI_DATA_NONE;
      sCommand.DummyCycles = 0;

      HAL_QSPI_Abort (hqspi_);

      // Enable write
      sCommand.Instruction = WRITE_ENABLE;
      if (HAL_QSPI_Command (hqspi_, &sCommand, QSPI_TIMEOUT) == HAL_OK)
	{
	  // Initiate erase
	  sCommand.Instruction = which;
	  sCommand.AddressMode = (which == CHIP_ERASE) ? QSPI_ADDRESS_NONE : //
	      QSPI_ADDRESS_1_LINE;
	  sCommand.DataMode = QSPI_DATA_NONE;
	  sCommand.Address = address;
	  if (HAL_QSPI_Command (hqspi_, &sCommand, QSPI_TIMEOUT) == HAL_OK)
	    {
	      // Set auto-polling and wait for the event
	      sCommand.Instruction = READ_STATUS_REGISTER;
	      sCommand.AddressMode = QSPI_ADDRESS_NONE;
	      sCommand.DataMode = QSPI_DATA_1_LINE;
	      sConfig.Match = 0;
	      sConfig.Mask = 1;
	      sConfig.MatchMode = QSPI_MATCH_MODE_AND;
	      sConfig.StatusBytesSize = 1;
	      sConfig.Interval = 0x10;
	      sConfig.AutomaticStop = QSPI_AUTOMATIC_STOP_ENABLE;
	      if (HAL_QSPI_AutoPolling_IT (hqspi_, &sCommand, &sConfig)
		  == HAL_OK)
		{
		  if (semaphore_.timed_wait (
		      (which == CHIP_ERASE) ? QSPI_CHIP_ERASE_TIMEOUT : //
			  QSPI_ERASE_TIMEOUT) == rtos::result::ok)
		    {
		      result = true;
		    }
		}
	    }
	}
      if (result == false)
	{
	  HAL_QSPI_Abort (hqspi_);
	}
      mutex_.unlock ();
    }
  return result;
}

/**
 * @brief  QSPI peripheral interrupt call-back.
 */
void
qspi::cb_event (void)
{
  semaphore_.post ();
}
