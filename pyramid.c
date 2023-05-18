uint32_t get_value(uint8_t row, uint8_t index)
{
    if (index > row)
    {
        printf("[ERROR] LIMIT\n");
        return -1;
    }
    else
    {
        int prev = 1;
        if (index == 0)
        {
            printf("ROW[%d] INDEX[0] = 1\n", (int)row);
            return 1;
        }
        for (int i = 1; i <= row; i++)
        {
            int curr = (prev * (row - i + 1)) / i;
            if (index-- == 0)
            {
                printf("ROW[%d] INDEX[%d] = %d\n", (int)row, i - 1, prev);
                return curr;
            }
            prev = curr;
        }
    }
    printf("\n[ERROR] NEVER BE HERE\n");
    return -1;
}

int main(void)
{
    get_value(2, 1);
    get_value(4, 3);
    get_value(5, 0);
    get_value(5, 1);
    get_value(5, 2);
    get_value(5, 3);
    get_value(5, 4);
    get_value(5, 5);
}

/*
    ROW[2] INDEX[1] = 2
    ROW[4] INDEX[3] = 4
    ROW[5] INDEX[0] = 1
    ROW[5] INDEX[1] = 5
    ROW[5] INDEX[2] = 10
    ROW[5] INDEX[3] = 10
    ROW[5] INDEX[4] = 5
*/


void
iso14443a_crc(uint8_t *pbtData, size_t szLen, uint8_t *pbtCrc)
{
  uint32_t wCrc = 0x6363;

  do {
    uint8_t  bt;
    bt = *pbtData++;
    bt = (bt ^ (uint8_t)(wCrc & 0x00FF));
    bt = (bt ^ (bt << 4));
    wCrc = (wCrc >> 8) ^ ((uint32_t) bt << 8) ^ ((uint32_t) bt << 3) ^ ((uint32_t) bt >> 4);
  } while (--szLen);

  *pbtCrc++ = (uint8_t)(wCrc & 0xFF);
  *pbtCrc = (uint8_t)((wCrc >> 8) & 0xFF);
}
