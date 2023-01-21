asm("jmp kmain");


#define COLOR_CODE_ADDRESS 0x9000
#define IDT_TYPE_INTR 0x0E
#define PIC1_PORT 0x20
#define VIDEO_BUFFER_ADDRESS 0xb8000

#define CURSOR_PORT 0x3D4
#define VIDEO_WIDTH 80
#define GDT_CS 0x8

/* константы для команд posixtime, wintime */
#define MAX_POSIX_TIME 2147483647
#define MAX_WIN_TIME 9223372036854775807

typedef void (*intr_handler)();

__inline unsigned char inb(unsigned short port);
__inline void outb(unsigned short port, unsigned char data);
void intr_handler_default(void);
void intr_reg(int index, unsigned short segm_sel, unsigned short flags, intr_handler hndlr);
void intr_init(void);
void intr_start(void); 
void keyboard_handler(void);
void keyboard_init(void);
void keyboard_load(void);
void keyboard_get_sym(const unsigned char symbol_code);
void set_cursor(void);
void put_string(unsigned char *string_output);
void put_symbol(const unsigned char symbol_output);
void shift_string(void);
void clear_screen(void);
void color_init(void);
int convert_to_numberMod(const unsigned char* numeral_string, const unsigned char conv_lim, short* pModNumber);
int convert_to_number32(const unsigned char* numeral_string, const unsigned char conv_lim);
int convert_to_NS(const unsigned char* numeral_string, const unsigned char conv_lim, const unsigned char NS);
short support_check(const unsigned char symbol, const unsigned char left, const unsigned char right);
short string_compare(const unsigned char *str1, const unsigned char *str2, const unsigned char length);
void shut_down(void);
void show_info(void);
short posix_win_time(unsigned char *cmd_line, const unsigned short firstYear);
short conv_NS_num(unsigned char* cmd_line);
void cmd_execute(void);


/* коды ошибок */
#define OVERFLOW_CODE -20 // переполнение
#define INVALID_SYMBOL_CODE -19 // неверный символ
#define INVALID_NS_CODE -18 // неверная СС
#define INVALID_CMD_CODE -17 // неверная команда
#define INVALID_DATA_CODE -16 // неверная дата
#define INVALID_ARG_CODE -15 // неверный аргумент
#define SUCCESS_CODE 0 // успех

const unsigned char ALPHABET[] = "0123456789abcdefghijklmnopqrstuvwxyz";
const unsigned char SUPPORTED_SYMBOLS[] = "\0\0001234567890\0\0\0\0qwertyuiop\0\0\0\0asdfghjkl\0\0\0\0\0zxcvbnm\0\0\0\0\0\0 ";

unsigned char CURRENT_TIME[] = "00.00.0000 00:00:00";
unsigned char ROW;
unsigned char COL;

//#pragma pack(push, 1)

struct idt_entry
{
    unsigned short base_lo;
    unsigned short segm_sel;
    unsigned char always0;
    unsigned char flags;
    unsigned short base_hi;
}__attribute__((packed));

struct idt_ptr
{
    unsigned short limit;
    unsigned int base;
}__attribute__((packed));

//#pragma pack(pop)

struct idt_entry g_idt[256];
struct idt_ptr g_idtp;

__inline unsigned char inb(unsigned short port)
{
    unsigned char data;
    asm("inb %w1, %b0"
        
        : "=a" (data)
	: "Nd" (port));

    return data;
}

__inline void outb(unsigned short port, unsigned char data)
{
    asm("outb %b0, %w1"
	  :: "a" (data), "Nd" (port));

}

//__declspec( naked ) 
void intr_handler_default()
{
    asm ("pusha");

    asm
    (
        "popa\n\t"
	"leave\n\t"
        "iret\n\t"
    );
}

void intr_reg(int index, unsigned short segm_sel, unsigned short flags, intr_handler hndlr)
{
    unsigned int hndlr_addr = (unsigned int)hndlr;
    g_idt[index].base_lo = (unsigned short)(hndlr_addr & 0xFFFF);
    g_idt[index].segm_sel = segm_sel;
    g_idt[index].always0 = 0;
    g_idt[index].flags = flags;
    g_idt[index].base_hi = (unsigned short)(hndlr_addr >> 16);
}

void intr_init()
{
   unsigned count = sizeof(g_idt) / sizeof(g_idt[0]);
    for (int i = 0; i < count; i++)
        intr_reg(i, GDT_CS, 0x80 | IDT_TYPE_INTR, intr_handler_default);
}

void intr_start()
{
    g_idtp.base = (unsigned int)(&g_idt[0]);
    g_idtp.limit = (sizeof(struct idt_entry) * sizeof(g_idt) / sizeof(g_idt[0])) - 1;
    asm ("lidt %0" : : "m" (g_idtp));
}


/* обработка клавиатуры */
//__declspec( naked ) 
void keyboard_handler()
{
    asm ("pusha");

    keyboard_load();
    outb(PIC1_PORT, 0x20);

    asm
    (
        "popa\n\t"
	"leave\n\t"
        "iret\n\t"
    );
}

/* инициализация клавиатуры */
void keyboard_init()
{
    intr_reg(0x09, GDT_CS, 0x80 | IDT_TYPE_INTR, keyboard_handler);
    outb(PIC1_PORT + 1, 0xFF ^ 0x02);
}

/* обработка символов с клавиатуры */
void keyboard_load()
{
    if (inb(0x64) & 0x01)
    {
        unsigned char symbol_code = inb(0x60);
        if (symbol_code <= 57)
            keyboard_get_sym(symbol_code);
    }
}

/* получение символа с клавиатуры */
void keyboard_get_sym(const unsigned char symbol_code)
{
    /* обработка клавиши BackSpace */
    if (symbol_code == 14) {
	    if (COL) {
                unsigned char* video_buf = (unsigned char*)VIDEO_BUFFER_ADDRESS + 2 * (VIDEO_WIDTH * ROW + COL);
                video_buf[0] = 0;
                COL--;
                set_cursor();
            }
        else
            return;
    }
    /* обработка клавиши Space */
    else if (symbol_code == 28) {
    	unsigned char* video_buf = (unsigned char*)VIDEO_BUFFER_ADDRESS + 2 * (VIDEO_WIDTH * ROW + COL);
        video_buf[0] = 0;
        COL = 0;
        shift_string();
        set_cursor();
        cmd_execute();
    }
    /* выводим символ с соответствующим кодом */
    else
        put_symbol(SUPPORTED_SYMBOLS[symbol_code]);
}

/* установка курсора на текущую позицию */
void set_cursor()
{
    unsigned char* video_buf = (unsigned char*)VIDEO_BUFFER_ADDRESS + 2 * (VIDEO_WIDTH * ROW + COL);
    video_buf[0] = ' ';

    outb(CURSOR_PORT, 0x0F);
    outb(CURSOR_PORT + 1, (unsigned char)(((ROW * VIDEO_WIDTH) + COL) & 0xFF));
    outb(CURSOR_PORT, 0x0E);
    outb(CURSOR_PORT + 1, (unsigned char)((((ROW * VIDEO_WIDTH) + COL) >> 8) & 0xFF));
}

/* вывод строки на экран */
void put_string(unsigned char *string_output)
{
    unsigned char *video_buf = (unsigned char *)VIDEO_BUFFER_ADDRESS + 2 * VIDEO_WIDTH * ROW;
    for (int i = 0, j = 0; string_output[i] != 0; j += 2)
        video_buf[j] = string_output[i++];

    COL = 0;
    shift_string();
    set_cursor();
}

/* вывод символа на экран */
void put_symbol(const unsigned char symbol_output)
{
    if (COL != 40) {
        unsigned char *video_buf = (unsigned char *)VIDEO_BUFFER_ADDRESS + 2 * (VIDEO_WIDTH * ROW + COL);
        video_buf[0] = symbol_output;
        COL++;
        set_cursor();
    }
    else
        return;
}

/* перенос строки */
void shift_string()
{
    if (ROW != 23) {
        ROW++;
	return;
    }

    unsigned char* video_buf = (unsigned char*)VIDEO_BUFFER_ADDRESS;
    for (unsigned char i = 0; i < ROW; i++) {
        for (unsigned char j = 0; j < 2 * VIDEO_WIDTH; j += 2)
            video_buf[j] = video_buf[2 * VIDEO_WIDTH + j];
        video_buf += 2 * VIDEO_WIDTH;
    }
    for (unsigned char i = 0; i < 2 * VIDEO_WIDTH; i += 2)
        video_buf[i] = 0;

}

/* очистка экрана */
void clear_screen()
{
    unsigned char* video_buf = (unsigned char*)VIDEO_BUFFER_ADDRESS;
    for (unsigned short i = 0; i < 48 * VIDEO_WIDTH; i += 2)
        video_buf[i] = 0;
    ROW = COL = 0;
    set_cursor();
}

/* выбор цвета ОС */
void color_init()
{
    unsigned char color_cur = 0;
    unsigned char color_code = *(unsigned char*)COLOR_CODE_ADDRESS;

    if (color_code == 0x01)
        color_cur = 0x04;
    else if (color_code == 0x02)
        color_cur = 0x02;
    else if (color_code == 0x03)
        color_cur = 0x01;
    else if (color_code == 0x04)
        color_cur = 0x0E;
    else if (color_code == 0x06)
        color_cur = 0x08;
    else
        color_cur = 0x07;

    unsigned char* video_buf = (unsigned char*)VIDEO_BUFFER_ADDRESS;
    for (unsigned short i = 1; i < 48 * VIDEO_WIDTH; i += 2)
        video_buf[i] = color_cur;
}

int convert_to_numberMod(const unsigned char* numeral_string, const unsigned char conv_lim, short* pModNumber)
{
    *pModNumber = (short)numeral_string[conv_lim - 1] - 48;
    if (conv_lim > 1)
        *pModNumber += 10 * ((short)numeral_string[conv_lim - 2] - 48);
    int result = 0;
    unsigned char i = 0;
    while (i < conv_lim - 2 && result >= 0) {
        result = 10 * result + ((int)numeral_string[i] - 48);
        i++;
    }
    return result;
}

int convert_to_number32(const unsigned char* numeral_string, const unsigned char conv_lim)
{
    int result = 0;
    unsigned char i = 0;
    while (i < conv_lim && result >= 0) {
        result = 10 * result + ((int)numeral_string[i] - 48);
	    i++;
    }
    return result;
}

int convert_to_NS(const unsigned char* numeral_string, const unsigned char conv_lim, const unsigned char NS)
{
    int result = 0;
    for (unsigned char i = 0, j = 0; i < conv_lim && result >= 0; i++, j = 0)
    {
        while (j < NS && numeral_string[i] != ALPHABET[j++]);
        if (numeral_string[i] != ALPHABET[--j])
            return INVALID_NS_CODE;
        result = NS * result + j;
    }
    return result;
}

/* проверка поддерживаемых символов */
short support_check(const unsigned char symbol, const unsigned char left, const unsigned char right)
{
    unsigned char i = left;
    while (i <= right) {
        if (symbol == ALPHABET[i])
            return SUCCESS_CODE;
        i++;
    }
    return INVALID_SYMBOL_CODE;
}

/* сравнение строк
   необходимо для обработчика команд*/
short string_compare(const unsigned char *str1, const unsigned char *str2, const unsigned char length)
{
    unsigned char i = 0;
    while (i < length) {
        if (str1[i] != str2[i])
            return INVALID_CMD_CODE;
        i++;
    }
    return SUCCESS_CODE;
}

/* выключение системы */
void shut_down()
{
    asm
    (
        "push %dx\n\t"
        "movw $0x604, %dx\n\t"
        "movw $0x2000, %ax\n\t"
        "outw %ax, %dx\n\t"
        "pop %dx\n\t"
    );
}


/* вывод на экран информации об ОС, ее разработчике и средств разработки */
void show_info()
{
    put_string((unsigned char*)"Lomaev Ivan, 4831001/00002, IKiZI, SPbPU, 03.2022, GCC compiler | YASM, Intel");
    put_string((unsigned char*)"Selected console color is:");
    unsigned char color_code = *(unsigned char*)COLOR_CODE_ADDRESS;
    if (color_code == 0x01)
        put_string((unsigned char*)"Red");
    else if (color_code == 0x02)
        put_string((unsigned char*)"Green");
    else if (color_code == 0x03)
        put_string((unsigned char*)"Blue");
    else if (color_code == 0x04)
        put_string((unsigned char*)"Yellow");
    else if (color_code == 0x06)
        put_string((unsigned char*)"Gray");
    else
        put_string((unsigned char*)"White");
}

/* печать получившегося времени */
void print_time(unsigned short* time)
{
    const unsigned char stringTemplateShifts[6] = { 0, 3, 6, 11, 14, 17 };
    unsigned char i = 0;
    while (i < 6) {
        int conv_lim = i == 2 ? 4 : 2;
        for (unsigned char j = 0; j < conv_lim; j++)
        {
            CURRENT_TIME[stringTemplateShifts[i] + conv_lim - j - 1] = (unsigned char)(time[i] % 10 + 48);
            time[i] /= 10;
        }
        i++;
    }

    put_string(CURRENT_TIME);

    i = 0;
    while (i < 6) {
        int conv_lim = i == 2 ? 4 : 2;
        for (unsigned char j = 0; j < conv_lim; j++)
            CURRENT_TIME[stringTemplateShifts[i] + j] = 48;
        i++;
    }
}

/* обработка команд posixtime, wintime */
short posix_win_time(unsigned char *cmd_line, const unsigned short firstYear)
{
    /* если командная строка пустая
       возвращаем код ошибки неверного аргумента*/
    if (!cmd_line[0])
        return INVALID_ARG_CODE;

    unsigned short time_cur[6] = { 1, 1, firstYear, 0, 0, 0 };
    unsigned char* newShift = cmd_line;
    short result = SUCCESS_CODE;

    while (result == SUCCESS_CODE)
        result = support_check(*newShift++, 0, 9);
    

    if (*(newShift - 1))
        return INVALID_SYMBOL_CODE;

    /* если стоит дата 1.01.1601
       переходим на печать*/
    if (firstYear == 1601 && newShift - cmd_line - 1 <= 7) {
        print_time(time_cur);
	return SUCCESS_CODE;
    }

    short time_remainingMod = 0;
    int time_remaining = convert_to_numberMod(cmd_line, firstYear == 1601 ? newShift - cmd_line - 8 : newShift - cmd_line - 1, &time_remainingMod);

    /* если при подсчетах вышли за допустимые пределы дат
       возвращаем код ошибки переполнения */
    if (time_remaining < 0 || (firstYear != 1601 && (time_remaining > 21474836 || (time_remaining == 21474836 && time_remainingMod > 47))))
        return OVERFLOW_CODE;

    /* если получилась дата ранее 1.01.1601
       возвращаем код ошибки неверной даты */
    if (firstYear == 1601 && (time_remaining > 157469184 || time_remaining == 157469184 && time_remainingMod >= 0))
        return INVALID_DATA_CODE;


    /* подсчет количества лет */
    while (time_remaining)
    {
        unsigned int sec_per_year = (time_cur[2] % 4 == 0 && time_cur[2] % 100 != 0) || (time_cur[2] % 400 == 0) ? 316224 : 315360;
	if (time_remaining < sec_per_year)
	    break;
        time_remaining -= sec_per_year;
        time_cur[2]++;
    }

    /* если расчеты произведены
       переходим на печать*/
    if (!(time_remaining || time_remainingMod)) {
        print_time(time_cur);
	return SUCCESS_CODE;
    }


    /* из оставшихся секунд считаем количество месяцев */
    const int sec_per_month[12] = { 26784, (time_cur[2] % 4 == 0 && time_cur[2] % 100 != 0) || (time_cur[2] % 400 == 0) ? 25056 : 24192, 26784,
                             25920, 26784, 25920, 26784, 26784, 25920, 26784, 25920, 26784 };

    unsigned short i = 0;

    while (i < 12 && time_remaining)
    {
        if (time_remaining < sec_per_month[i])
            break;
        time_remaining -= sec_per_month[i];
        time_cur[1]++;
	i++;
    }

    /* если расчеты произведены
       переходим на печать*/
    if (!(time_remaining || time_remainingMod)) {
        print_time(time_cur);
	return SUCCESS_CODE;
    }

    /* из оставшихся секунд считаем количество дней */
    time_cur[0] += time_remaining / 864;
    time_remaining %= 864;
    time_remaining = 100 * time_remaining + time_remainingMod;

    /* если расчеты произведены
       переходим на печать*/
    if (!(time_remaining || time_remainingMod)) {
        print_time(time_cur);
	return SUCCESS_CODE;
    }

    /* из оставшихся секунд считаем точное время */
    int sec_per = 3600;
    i = 3;
    while (time_remaining >= 1)
    {
        time_cur[i] += time_remaining / sec_per;
        time_remaining %= sec_per;
        sec_per /= 60;
	i++;
    }


    print_time(time_cur);
    return SUCCESS_CODE;
}

/* обработка команды nsconv */
short conv_NS_num(unsigned char* cmd_line)
{
    if (*cmd_line == '\0')
        return INVALID_ARG_CODE;

    unsigned char* numberShifts[4];
    numberShifts[0] = cmd_line;

    int result = SUCCESS_CODE;

    unsigned short i = 1;

    while (i < 4)
    {
        numberShifts[i] = numberShifts[i - 1];
        if (i > 1)
            do
                result = support_check(*numberShifts[i]++, 0, 9);
	    while (!result);
        else
            do
                result = support_check(*numberShifts[i]++, 0, 35);
	    while (!result);
        if ((*(numberShifts[i] - 1) != ' ' && i != 3) || (i == 3 && *(numberShifts[i] - 1) != '\0'))
            return INVALID_ARG_CODE;
        i++;
    }

    unsigned char start_NS = convert_to_number32(numberShifts[1], numberShifts[2] - (numberShifts[1] + 1)), new_NS = convert_to_number32(numberShifts[2], numberShifts[3] - (numberShifts[2] + 1));

    if (!(start_NS >= 2 && start_NS <= 36) || !(new_NS >= 2 && new_NS <= 36))
        return INVALID_NS_CODE;

    result = convert_to_NS(numberShifts[0], numberShifts[1] - (numberShifts[0] + 1), start_NS);

    if (result < 0)
        return result == INVALID_NS_CODE ? result : OVERFLOW_CODE;

    int number_in_NS = result;
    unsigned char count = 0;
    while (number_in_NS /= new_NS)
        count++;
    number_in_NS = result;

    unsigned char numeral_string[] = { "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" };

    i = 0;
    while (i <= count)
    {
        numeral_string[count - i] = ALPHABET[number_in_NS % new_NS];
        number_in_NS /= new_NS;
        i++;
    }

    put_string(numeral_string);

    return SUCCESS_CODE;
}

/* обработка текущей команды */
void cmd_execute()
{
    unsigned char *video_buf = (unsigned char *)VIDEO_BUFFER_ADDRESS + 2 * VIDEO_WIDTH * (ROW - 1);
    unsigned char cmd_line[] = { "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" };
    for (unsigned char i = 0, j = 0; video_buf[i]; i += 2)
        cmd_line[j++] = video_buf[i];

    short result = SUCCESS_CODE;

	/* обработка команд */


	/* обработка команды clear */
    if (*video_buf == 'c') {
        result = string_compare(cmd_line, (unsigned char*)"clear", 6);
        if (result == SUCCESS_CODE) {
            clear_screen();
            return;
        }
    }

	/* обработка команды shutdown */
    else if (*video_buf == 's') {
        result = string_compare(cmd_line, (unsigned char*)"shutdown", 9);
        if (result == SUCCESS_CODE) {
            shut_down();
            return;
        }
    }


	/* обработка команды info */
    else if (*video_buf == 'i') {
        result = string_compare(cmd_line, (unsigned char*)"info", 5);
        if (result == SUCCESS_CODE) {
            show_info();
        }
    }

	/* обработка команды posixtime */
    else if (*video_buf == 'p') {
        result = string_compare(cmd_line, (unsigned char*)"posixtime ", 10);
        if (result == SUCCESS_CODE) {
            result = posix_win_time(cmd_line + 10, 1970);
        }
    }

	/* обработка команды wintime */
    else if (*video_buf == 'w') {
        result = string_compare(cmd_line, (unsigned char*)"wintime ", 8);
        if (result == SUCCESS_CODE) {
            result = posix_win_time(cmd_line + 8, 1601);
        }
    }

	/* обработка команды nscov */
    else if (*video_buf == 'n') {
        result = string_compare(cmd_line, (unsigned char*)"nsconv ", 7);
        if (result == SUCCESS_CODE) {
            result = conv_NS_num(cmd_line + 7);
        }
    }

	/* если ни одна из этих команд не обработалась */
    else
        result = INVALID_CMD_CODE;
	
	 /* обработка ошибок */

	/* если произошло переполнение */
	if (result == OVERFLOW_CODE)
        put_string((unsigned char*)"Error: Integer overflow");

	/* если был введен неправильный символ */
    else if (result == INVALID_SYMBOL_CODE)
        put_string((unsigned char*)"Error: Invalid argument symbol");
    
	/* если была введена неправильная команда */
    else if (result == INVALID_CMD_CODE)
        put_string((unsigned char*)"Error: Invalid command");

	/* если была введена неверная система счисления */
    else if (result == INVALID_NS_CODE)
        put_string((unsigned char*)"Error: Invalid numeral system");
    
	/* если дата вышла за границы */
    else if (result == INVALID_DATA_CODE)
        put_string((unsigned char*)"Error: Data is out of format. Max supported time is 31.12.2099 23:59:59");
    
	/* если был передан неверный аргумент */
    else if (result == INVALID_ARG_CODE)
        put_string((unsigned char*)"Error: Invalid command argument");
   

}

extern "C" int kmain()
{
    /* инициализация окна ОС */
    color_init();
    clear_screen();

    /* отключение прерываний */
    __asm("cli");

    /* вывод приветственного сообщения */
    put_string((unsigned char*)"Welcome to ConvertOS!"); 

    /* инициализация прерываний */
    intr_init(); 

    /* начало прерываний */
    intr_start(); 

    /* включение прерываний */
    __asm ("sti"); 

    /* инициализация клавиатуры */
    keyboard_init(); 

    /* активный режим работы ОС */
    while (1) 
        __asm ("hlt");
    return 0;
}
