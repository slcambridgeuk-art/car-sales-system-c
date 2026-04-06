#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>


#define MAX_MODELS 6

#define DISCOUNT_MULTIBUY_PERCENTAGE 0.15f
#define DISCOUNT_MULTIBUY_AMOUNT     3
#define DISCOUNT_MEMBER_PERCENTAGE   0.10f

#define SALES_FILE    "sales.dat"
#define FEEDBACK_FILE "feedback.dat"


#define USE_XOR_ENCRYPTION 0
#define XOR_KEY 0x5A

static const char *models[MAX_MODELS] = {
    "Toyota Yaris",
    "VW Golf",
    "BMW 3 Series",
    "Toyota Prius Hybrid",
    "Ford Explorer",
    "Mercedes E-Class"
};
static float prices[MAX_MODELS] = {
    13995.0f,
    19990.0f,
    27995.0f,
    22990.0f,
    33990.0f,
    38990.0f
};
static int years[MAX_MODELS] = {
    2022, 2024, 2023, 2021, 2022, 2024
};
static int stock[MAX_MODELS] = {
    9, 14, 6, 7, 5, 3
};

typedef struct {
    int   modelIndex;
    int   quantity;
    float unitPrice;
    int   discountGiven;
    float discountValue;
    float totalPrice;
    char  customerName[64];
    int   customerAge;
    char  date[11];
} Sale;

typedef struct {
    int  modelIndex;
    int  rating;
    char comment[128];
} Feedback;

typedef struct { Sale *items;     size_t count, cap; } SaleList;
typedef struct { Feedback *items; size_t count, cap; } FeedbackList;

static SaleList     g_sales     = { NULL, 0, 0 };
static FeedbackList g_feedbacks = { NULL, 0, 0 };


static void ensure_sales_capacity(size_t need){
    if (g_sales.cap >= need) return;
    size_t nc = g_sales.cap ? g_sales.cap * 2 : 16;
    while (nc < need) nc *= 2;
    Sale *p = (Sale*)realloc(g_sales.items, nc * sizeof(Sale));
    if (!p){ fprintf(stderr,"Out of memory (sales)\n"); exit(1); }
    g_sales.items = p; g_sales.cap = nc;
}
static void ensure_feedback_capacity(size_t need){
    if (g_feedbacks.cap >= need) return;
    size_t nc = g_feedbacks.cap ? g_feedbacks.cap * 2 : 16;
    while (nc < need) nc *= 2;
    Feedback *p = (Feedback*)realloc(g_feedbacks.items, nc * sizeof(Feedback));
    if (!p){ fprintf(stderr,"Out of memory (feedback)\n"); exit(1); }
    g_feedbacks.items = p; g_feedbacks.cap = nc;
}

static void xor_buf(unsigned char *buf, size_t n){
#if USE_XOR_ENCRYPTION
    for (size_t i = 0; i < n; ++i) buf[i] ^= XOR_KEY;
#else
    (void)buf; (void)n;
#endif
}

static void load_sales(void){
    FILE *f = fopen(SALES_FILE,"rb"); if(!f) return;
    fseek(f,0,SEEK_END); long sz = ftell(f); if(sz<=0){ fclose(f); return; }
    fseek(f,0,SEEK_SET);
    unsigned char *buf = (unsigned char*)malloc((size_t)sz);
    if(!buf){ fclose(f); return; }
    fread(buf,1,(size_t)sz,f); fclose(f);
    xor_buf(buf,(size_t)sz);

    size_t cnt = (size_t)sz / sizeof(Sale);
    ensure_sales_capacity(cnt);
    memcpy(g_sales.items, buf, cnt * sizeof(Sale));
    g_sales.count = cnt;
    free(buf);

    for (size_t i=0;i<g_sales.count;++i){
        int mi = g_sales.items[i].modelIndex;
        if (mi>=0 && mi<MAX_MODELS){
            stock[mi] -= g_sales.items[i].quantity;
            if (stock[mi] < 0) stock[mi] = 0;
        }
    }
}
static void save_sales(void){
    size_t nbytes = g_sales.count * sizeof(Sale);
    unsigned char *buf = (unsigned char*)malloc(nbytes ? nbytes : 1);
    if(!buf) return;
    if(nbytes) memcpy(buf, g_sales.items, nbytes);
    xor_buf(buf, nbytes);
    FILE *f = fopen(SALES_FILE,"wb"); if(!f){ free(buf); return; }
    if(nbytes) fwrite(buf,1,nbytes,f);
    fclose(f); free(buf);
}

static void load_feedbacks(void){
    FILE *f = fopen(FEEDBACK_FILE,"rb"); if(!f) return;
    fseek(f,0,SEEK_END); long sz = ftell(f); if(sz<=0){ fclose(f); return; }
    fseek(f,0,SEEK_SET);
    unsigned char *buf = (unsigned char*)malloc((size_t)sz);
    if(!buf){ fclose(f); return; }
    fread(buf,1,(size_t)sz,f); fclose(f);
    xor_buf(buf,(size_t)sz);

    size_t cnt = (size_t)sz / sizeof(Feedback);
    ensure_feedback_capacity(cnt);
    memcpy(g_feedbacks.items, buf, cnt * sizeof(Feedback));
    g_feedbacks.count = cnt;
    free(buf);
}
static void save_feedbacks(void){
    size_t nbytes = g_feedbacks.count * sizeof(Feedback);
    unsigned char *buf = (unsigned char*)malloc(nbytes ? nbytes : 1);
    if(!buf) return;
    if(nbytes) memcpy(buf, g_feedbacks.items, nbytes);
    xor_buf(buf, nbytes);
    FILE *f = fopen(FEEDBACK_FILE,"wb"); if(!f){ free(buf); return; }
    if(nbytes) fwrite(buf,1,nbytes,f);
    fclose(f); free(buf);
}

static void flush_line(void){ int c; while((c=getchar())!='\n' && c!=EOF){} }

static int read_int_in_range(const char *prompt,int min,int max){
    for(;;){
        printf("%s",prompt);
        int v; char ch;
        if (scanf("%d%c",&v,&ch)==2 && ch=='\n'){
            if (v>=min && v<=max) return v;
            printf("Please enter a number between %d and %d.\n",min,max);
        } else {
            printf("Invalid input. Whole numbers only.\n");
            flush_line();
        }
    }
}
static void read_string(const char *prompt,char *out,size_t outsz){
    for(;;){
        printf("%s",prompt);
        if (!fgets(out,(int)outsz,stdin)){ clearerr(stdin); continue; }
        size_t n=strlen(out); if(n&&out[n-1]=='\n') out[n-1]='\0';
        int blank=1; for(size_t i=0; out[i]; ++i) if(!isspace((unsigned char)out[i])){ blank=0; break; }
        if(!blank && out[0]!='\0') return;
        printf("Please enter something (not empty).\n");
    }
}
static int read_yes_no(const char *prompt){
    for(;;){
        printf("%s",prompt);
        char s[16];
        if(!fgets(s,sizeof(s),stdin)){ clearerr(stdin); continue; }
        if(s[0]=='\n'||s[0]=='\0') continue;
        char c=(char)toupper((unsigned char)s[0]);
        if(c=='Y') return 1;
        if(c=='N') return 0;
        printf("Please enter Y or N.\n");
    }
}

static void today_yyyy_mm_dd(char out[11]){
    time_t t=time(NULL); struct tm *lt=localtime(&t);
    strftime(out,11,"%Y-%m-%d",lt);
}

static void sort_indices_by_year_desc(int idx[],int n){
    for(int i=1;i<n;++i){
        int key=idx[i], j=i-1;
        while(j>=0){
            int a=years[idx[j]], b=years[key];
            int take = (b>a) || ((b==a) && (strcmp(models[key],models[idx[j]])<0));
            if(!take) break;
            idx[j+1]=idx[j]; --j;
        }
        idx[j+1]=key;
    }
}
static void sort_by_total_desc(int idx[], float totals[], int n){
    for (int i=1;i<n;++i){
        int key=idx[i], j=i-1;
        float tkey=totals[key];
        while(j>=0 && totals[idx[j]]<tkey){ idx[j+1]=idx[j]; --j; }
        idx[j+1]=key;
    }
}

static void action_view_stock(void){
    printf("\n=== Cars in Stock (newest first) ===\n");
    int order[MAX_MODELS]; for(int i=0;i<MAX_MODELS;++i) order[i]=i;
    sort_indices_by_year_desc(order, MAX_MODELS);

    printf("%-3s %-20s %-6s %-10s %-8s\n","#","Model","Year","UnitPrice","Left");
    for(int k=0;k<MAX_MODELS;++k){
        int i=order[k];
        printf("%-3d %-20s %-6d £%-9.2f %-8d\n", i+1, models[i], years[i], prices[i], stock[i]);
    }
}

static void action_buy_cars(void){
    /* any stock at all? */
    int any=0; for(int i=0;i<MAX_MODELS;++i) if(stock[i]>0){ any=1; break; }
    if(!any){ printf("Sorry—everything is sold out right now.\n"); return; }

    /* age & license */
    int age = read_int_in_range("Your age (must be 18+): ", 0, 130);
    if (age < 18){ printf("Sorry, you need to be 18 or older.\n"); return; }
    int hasLicense = read_yes_no("Do you have a valid driver's license? (Y/N): ");
    if (!hasLicense){ printf("We can only sell to licensed drivers. Sorry!\n"); return; }


    action_view_stock();

    int modelIndex = read_int_in_range("Pick a model by # (1-6): ", 1, MAX_MODELS) - 1;
    if (stock[modelIndex] <= 0){ printf("Ah, that one's out of stock. Try another.\n"); return; }

    char qtyPrompt[96];
    snprintf(qtyPrompt,sizeof(qtyPrompt),"How many '%s' would you like (1..%d)? ",
             models[modelIndex], stock[modelIndex]);
    int qty = read_int_in_range(qtyPrompt, 1, stock[modelIndex]);

    int isMember = read_yes_no("Are you a Car Club member? (Y/N): ");

    flush_line();
    char custName[64];
    read_string("Your full name: ", custName, sizeof(custName));

    float unitPrice = prices[modelIndex];
    float total = unitPrice * qty;
    int   discountGiven = 0;
    float discountVal = 0.0f;

    if (isMember){
        discountGiven = 1; discountVal = DISCOUNT_MEMBER_PERCENTAGE;
        total *= (1.0f - DISCOUNT_MEMBER_PERCENTAGE);
    } else if (qty >= DISCOUNT_MULTIBUY_AMOUNT){
        discountGiven = 1; discountVal = DISCOUNT_MULTIBUY_PERCENTAGE;
        total *= (1.0f - DISCOUNT_MULTIBUY_PERCENTAGE);
    }

    printf("\n--- Quick Summary ---\n");
    printf("Model: %s (%d)\n", models[modelIndex], years[modelIndex]);
    printf("Price each: £%.2f   Qty: %d\n", unitPrice, qty);
    if (discountGiven) printf("Discount: %.0f%%\n", discountVal*100.0f);
    else               printf("Discount: none\n");
    printf("TOTAL to pay: £%.2f\n", total);

    int confirm = read_yes_no("Go ahead with the purchase? (Y/N): ");
    if (!confirm){ printf("No problem—order cancelled.\n"); return; }

    stock[modelIndex] -= qty;

    ensure_sales_capacity(g_sales.count+1);
    Sale *s = &g_sales.items[g_sales.count++];
    s->modelIndex   = modelIndex;
    s->quantity     = qty;
    s->unitPrice    = unitPrice;
    s->discountGiven= discountGiven;
    s->discountValue= discountVal;
    s->totalPrice   = total;
    strncpy(s->customerName, custName, sizeof(s->customerName)-1);
    s->customerName[sizeof(s->customerName)-1] = '\0';
    s->customerAge  = age;
    today_yyyy_mm_dd(s->date);

    save_sales();

    printf("\nEnjoy your car! Want to leave quick feedback for %s?\n", models[modelIndex]);
    if (read_yes_no("Leave feedback now? (Y/N): ")){
        int rating = read_int_in_range("Rate 1..5: ", 1, 5);
        char comment[128];
        read_string("Short comment: ", comment, sizeof(comment));

        ensure_feedback_capacity(g_feedbacks.count+1);
        Feedback *fb = &g_feedbacks.items[g_feedbacks.count++];
        fb->modelIndex = modelIndex; fb->rating = rating;
        strncpy(fb->comment, comment, sizeof(fb->comment)-1);
        fb->comment[sizeof(fb->comment)-1] = '\0';
        save_feedbacks();
        printf("Thanks! We saved your feedback.\n");
    }
}

static void action_view_sales(void){
    if (g_sales.count == 0){ printf("\nNo sales yet.\n"); return; }

    printf("\n=== All Sales (when they happened) ===\n");
    printf("%-10s | %-20s | %-20s | %-3s | %-5s | %-9s | %-10s\n",
           "Date","Model","Customer","Age","Qty","Discount","Total£");
    for (size_t i=0;i<g_sales.count;++i){
        Sale *s = &g_sales.items[i];
        char disc[16];
        if (s->discountGiven) snprintf(disc,sizeof(disc),"%.0f%%", s->discountValue*100.0f);
        else strcpy(disc,"None");
        printf("%-10s | %-20s | %-20s | %-3d | %-5d | %-9s | %-10.2f\n",
               s->date, models[s->modelIndex], s->customerName, s->customerAge,
               s->quantity, disc, s->totalPrice);
    }

    float totalRevenue[MAX_MODELS]={0};
    int   totalUnits  [MAX_MODELS]={0};
    for (size_t i=0;i<g_sales.count;++i){
        totalRevenue[g_sales.items[i].modelIndex] += g_sales.items[i].totalPrice;
        totalUnits  [g_sales.items[i].modelIndex] += g_sales.items[i].quantity;
    }

    int idx[MAX_MODELS]; int n=0;
    for(int i=0;i<MAX_MODELS;++i) if(totalUnits[i]>0) idx[n++]=i;
    if (n==0){ printf("\n(No model sales yet to summarise.)\n"); return; }

    sort_by_total_desc(idx, totalRevenue, MAX_MODELS);

    printf("\n=== Sales Summary by Model (highest £ first) ===\n");
    printf("%-20s | %-5s | %-10s\n", "Model","Units","Total£");
    for(int k=0;k<MAX_MODELS;++k){
        int i=idx[k]; if(totalUnits[i]==0) continue;
        printf("%-20s | %-5d | %-10.2f\n", models[i], totalUnits[i], totalRevenue[i]);
    }
}


static void export_csv(void){

    FILE *fs = fopen("sales.csv","w");
    if (!fs){ printf("Couldn't write sales.csv\n"); return; }
    fprintf(fs, "Date,Model,Customer,Age,Qty,UnitPrice,DiscountPct,Total\n");
    for (size_t i=0;i<g_sales.count;++i){
        Sale *s=&g_sales.items[i];
        double disc = s->discountGiven ? (double)(s->discountValue*100.0f) : 0.0;
        fprintf(fs, "%s,%s,%s,%d,%d,%.2f,%.0f,%.2f\n",
                s->date, models[s->modelIndex], s->customerName, s->customerAge,
                s->quantity, s->unitPrice, disc, s->totalPrice);
    }
    fclose(fs);

    float revenue[MAX_MODELS]={0};
    int   units  [MAX_MODELS]={0};
    for (size_t i=0;i<g_sales.count;++i){
        revenue[g_sales.items[i].modelIndex] += g_sales.items[i].totalPrice;
        units  [g_sales.items[i].modelIndex] += g_sales.items[i].quantity;
    }
    FILE *fs2 = fopen("sales_summary.csv","w");
    if (!fs2){ printf("Couldn't write sales_summary.csv\n"); return; }
    fprintf(fs2, "Model,Units,Total\n");
    for (int i=0;i<MAX_MODELS;++i){
        if (units[i]==0) continue;
        fprintf(fs2, "%s,%d,%.2f\n", models[i], units[i], revenue[i]);
    }
    fclose(fs2);

    printf("\nExported:\n  - sales.csv (all sales)\n  - sales_summary.csv (per-model totals)\n");
}

static void action_feedback(void){
    if (g_feedbacks.count == 0){ printf("\nNo feedback yet.\n"); return; }
    printf("\n=== Customer Feedback (by model) ===\n");
    for (int m=0;m<MAX_MODELS;++m){
        int header=0;
        for (size_t i=0;i<g_feedbacks.count;++i){
            Feedback *fb=&g_feedbacks.items[i];
            if (fb->modelIndex==m){
                if(!header){ printf("\n[%s]\n", models[m]); header=1; }
                printf("  %d/5  —  \"%s\"\n", fb->rating, fb->comment);
            }
        }
    }
}

static void press_enter_to_continue(void){
    printf("\nPress ENTER to continue...");
    flush_line();
}

static void main_menu(void){
    for(;;){
        printf("\n---------------------------------------\n");
        printf(" Welcome to the Car Sales Desk\n");
        printf("---------------------------------------\n");
        printf(" 1) View Cars Stock\n");
        printf(" 2) Buy Cars\n");
        printf(" 3) View Sales Data\n");
        printf(" 4) Customer Feedback\n");
        printf(" 5) Export Sales CSV\n");
        printf(" 0) Exit\n");
        int c = read_int_in_range("Pick an option: ", 0, 5);
        switch(c){
            case 1: action_view_stock(); press_enter_to_continue(); break;
            case 2: action_buy_cars();   press_enter_to_continue(); break;
            case 3: action_view_sales(); press_enter_to_continue(); break;
            case 4: action_feedback();   press_enter_to_continue(); break;
            case 5: export_csv();        press_enter_to_continue(); break;
            case 0: return;
        }
    }
}

int main(void){
    load_sales();
    load_feedbacks();
    printf("Hello! If you’ve used this before, we’ve loaded your past sales and feedback.\n");
    main_menu();
    save_sales();
    save_feedbacks();
    printf("\nThanks for stopping by. Bye!\n");
    return 0;
}

