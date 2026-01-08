#pragma once

#include <stdio.h>
#include <stdint.h>

/**
 * 5 ROWS [0..=4]
 */
#define GPIO_ROWS \
{ 0, GPIO_NUM_4 }, \
{ 1, GPIO_NUM_5 }, \
{ 2, GPIO_NUM_13 }, \
{ 3, GPIO_NUM_14 }, \
{ 4, GPIO_NUM_15 }, \
// { 5, GPIO_NUM_16 }, \ (reserved for future layouts)
// { 6, GPIO_NUM_17 }, \ (reserved for future layouts)

/**
 * 13 COLS [0..=12]
 */
#define GPIO_COLS \
{ 0, GPIO_NUM_18 }, \
{ 1, GPIO_NUM_19 }, \
{ 2, GPIO_NUM_21 }, \
{ 3, GPIO_NUM_22 }, \
{ 4, GPIO_NUM_23 }, \
{ 5, GPIO_NUM_25 }, \
{ 6, GPIO_NUM_26 }, \
{ 7, GPIO_NUM_27 }, \
{ 8, GPIO_NUM_32 }, \
{ 9, GPIO_NUM_33 }, \
{ 10, GPIO_NUM_35 }, \
{ 11, GPIO_NUM_36 }, \
{ 12, GPIO_NUM_39 }, \

void scan(uint8_t* out_active_rc_pairs);