#include <time.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ltmodule.h"

#ifndef NULL
#define NULL __null
#endif

#ifndef BOOL
#define BOOL bool
#endif


typedef unsigned long DWORD;

#ifndef PACK_INF
#define Pks_Payload_Len 1000
#define Pks_Header_Len 10
#define Pks_Len 1010
#define Data_Pks_Header   0x47
#endif

#define Seed_Buffer_Len 100000

namespace ns3 {

typedef struct StructList
{
	uint32_t data;
	struct StructList *next;
}LIST,*PLIST;

class BP_Index // Believe Propagation (BP) structure
{
public:
	BP_Index(uint32_t Len);
	~BP_Index();
	void Ins_BP_Index(uint32_t index,uint32_t data);
	BOOL Del_BP_Index(uint32_t index,uint32_t data);
	uint32_t Get_Del_head(uint32_t index);
public:
	PLIST *m_index;
	uint32_t *m_degree;
};

BP_Index::BP_Index(uint32_t Len)
{
	m_index=new PLIST[Len];
	m_degree=new uint32_t [Len];
	uint32_t i;
	for (i=0;i<Len;i++)
	{
		m_index[i]=NULL;
	}
	for (i=0;i<Len;i++)
	{
		m_degree[i]=0;
	}
}
BP_Index::~BP_Index()
{
	delete []m_index;
	m_index=NULL;
	delete []m_degree;
	m_degree=NULL;
}
void BP_Index::Ins_BP_Index(uint32_t index,uint32_t data)
{
	PLIST P_list=m_index[index-1];
	if (P_list==NULL)
	{
		m_index[index-1]=new LIST;
		m_index[index-1]->data=data;
		m_index[index-1]->next=NULL;
	}
	else
	{
		PLIST p_new_list=new LIST;
		p_new_list->data=data;
		p_new_list->next=P_list;
		m_index[index-1]=p_new_list;		
	}
	m_degree[index-1]++;
}
BOOL BP_Index::Del_BP_Index(uint32_t index,uint32_t data)
{
	PLIST P_list=m_index[index-1];
	PLIST p_back=NULL;
	PLIST p_now=P_list;
	if (p_now==NULL)
	{
		return false;
	}
	uint32_t now_data=p_now->data;
	if (now_data==data)
	{	
		m_index[index-1]=P_list->next;
		delete p_now;
		m_degree[index-1]--;
		return true;
	}
	while ((now_data!=data)&&(p_now!=NULL))
	{
		p_back=p_now;
		p_now=p_now->next;
		if (p_now!=NULL)
		{
			now_data=p_now->data;
		}	
	}
	if (p_now==NULL)
	{
		return false;
	}
	p_back->next=p_now->next;
	delete p_now;
	m_degree[index-1]--;
	return true;
}
uint32_t BP_Index::Get_Del_head(uint32_t index)
{
	PLIST P_list=m_index[index-1];
	PLIST P_foreward=P_list->next;
	uint32_t data=P_list->data;
	delete P_list;
	m_index[index-1]=P_foreward;
	m_degree[index-1]--;
	return data;
}

class List // a normal list
{
public:
	List();
	~List();
	void Ins_list(uint32_t data);
	uint32_t Get_Del_head(void);
	BOOL Del_list(uint32_t data);
	void Clear(void);
public:
	PLIST m_list;
	uint32_t  m_Len;
	
};

List::List()
{
	m_list=NULL;
	m_Len=0;
}
List::~List()
{
	if (m_list)
	{
		delete m_list;
		m_list=NULL;
	}
	m_Len=0;
}

void List::Ins_list(uint32_t data)
{
	if (m_list==NULL)
	{
		m_list=new LIST;
		m_list->data=data;
		m_list->next=NULL;
	}
	else
	{
		PLIST m_new_list=new LIST;
		m_new_list->data=data;
		m_new_list->next=m_list;
		m_list=m_new_list;		
	}
	m_Len++;
}
uint32_t List::Get_Del_head(void)
{
	PLIST P_list=m_list;
	if (P_list!=NULL)
	{
		PLIST P_foreward=P_list->next;
		uint32_t data=P_list->data;
		delete P_list;
		m_list=P_foreward;
		m_Len--;
		return data;
	}
	return 0;

}
BOOL List::Del_list(uint32_t data)
{
	PLIST p_back=NULL;
	PLIST p_now=m_list;
	if (p_now==NULL)
	{
		return false;
	}
	uint32_t now_data=p_now->data;
	if (now_data==data)
	{	
		m_list=m_list->next;
		delete p_now;
		m_Len--;
		return true;
	}
	while ((now_data!=data)&&(p_now!=NULL))
	{
		p_back=p_now;
		p_now=p_now->next;
		if (p_now!=NULL)
		{
			now_data=p_now->data;
		}	
	}
	if (p_now==NULL)
	{
		return false;
	}
	p_back->next=p_now->next;
	delete p_now;
	m_Len--;
	return true;

}
void List::Clear(void)
{
	if (m_list)
	{
		delete m_list;
		m_list=NULL;
	}
	m_Len=0;
}



class Seed_Pro //this is the class to generate random seed
{
public:
	Seed_Pro(uint32_t len);
	~Seed_Pro(void);
	void Rand_Seed(void);
	uint32_t Get_Seed(void);
protected:
private:
	uint32_t *Seed_Buf;
	uint32_t Seed_Buf_Len;
	uint32_t P_top;
};

Seed_Pro::Seed_Pro(uint32_t len)
{
	Seed_Buf_Len=len;
	P_top=0;
	Seed_Buf=new uint32_t[Seed_Buf_Len];
	Rand_Seed();
}
Seed_Pro::~Seed_Pro(void)
{
	delete Seed_Buf;
}
void Seed_Pro::Rand_Seed()
{
	uint32_t i;
	srand( (unsigned)time( NULL )); // use the system time to initialize the random generator
	for (i=0;i<Seed_Buf_Len;i++)
	{
		Seed_Buf[i]=rand()*10000;
	}
}
uint32_t Seed_Pro::Get_Seed() // get random seed
{
	if (P_top==Seed_Buf_Len)
	{
		Rand_Seed();
		P_top=0;
	}
	P_top++;
	return Seed_Buf[P_top-1];
}

uint32_t sum(uint32_t*data,uint32_t len) //calculate sum
{
	uint32_t anssum=0;
	uint32_t i;
	for (i=0;i<len;i++)
	{
		anssum+=data[i];
	}
	return anssum;
}

double* Degree(uint32_t Ori_Pks_Num,double c,double omega) // generate the degree distribution according the Robust Soliton distribution
{
	double * robust_d=new double[Ori_Pks_Num];
	memset(robust_d,0,sizeof(double)*Ori_Pks_Num);
	double *P=new double[Ori_Pks_Num];
	memset(P,0,sizeof(double)*Ori_Pks_Num);
	double *t=new double[Ori_Pks_Num];
	memset(t,0,sizeof(double)*Ori_Pks_Num);
	P[0]=1/(double)Ori_Pks_Num;
	uint32_t i=0;
	for (i=1;i<Ori_Pks_Num;i++)
	{
		P[i]=1/(double)(i*(i+1));
	}
	double s=c*log(Ori_Pks_Num/omega)*sqrt(Ori_Pks_Num);
	for (i=0;i<ceil(Ori_Pks_Num/s)-1;i++)
	{
		t[i]=s/(Ori_Pks_Num*(i+1));
	}
	t[(uint32_t)ceil(Ori_Pks_Num/s)-1]=s/Ori_Pks_Num*log(s/omega);
	double sum=0;
	for (i=0;i<Ori_Pks_Num;i++)
	{
		robust_d[i]=P[i]+t[i];
		sum+=robust_d[i];
	}
	for (i=0;i<Ori_Pks_Num;i++)
	{
		robust_d[i]=robust_d[i]/sum;
		if (i)
		{
			robust_d[i]=robust_d[i-1]+robust_d[i];
		}		
	}
	delete []P;
	delete []t;
	return robust_d;
}

BOOL Repeat(uint32_t *Neighbors_I,uint32_t Neighbors_count) // check repeated or not
{
	uint32_t i;
	for (i=0;i<Neighbors_count;i++)
	{
		if (Neighbors_I[i]==Neighbors_I[Neighbors_count])
		{
			return true;
		}
		
	}
	return false;
}

//the encoding function
void LT_Encode(char* buf_send,char* buf,DWORD dwFileSize,DWORD LTSendSize)
{
	uint32_t Ori_Pks_Num=ceil((double)dwFileSize/(double)Pks_Payload_Len);
	uint32_t Enc_Pks_Num=ceil((double)LTSendSize/(double)Pks_Len);

	class Seed_Pro seed(Seed_Buffer_Len);
	uint32_t i;
	double *degree=Degree(Ori_Pks_Num,0.01,0.5);
	char *Pks_I=new char [Pks_Len];// a new packet
	char *Pks_read_I=new char[Pks_Payload_Len];
	for (i=0;i<Enc_Pks_Num;i++)
	{
		*(uint16_t*)Pks_I = Data_Pks_Header;
		*(uint32_t*)(Pks_I+2)=dwFileSize;//file length
		uint32_t Seed_Once=seed.Get_Seed();
		*(uint32_t*)(Pks_I+6)=Seed_Once;
		srand( Seed_Once);
		double Rand_Num=(double)rand()/((double)RAND_MAX+1);
		uint32_t degree_I=0;
		uint32_t j=0;
		for (j=0;j<Ori_Pks_Num;j++)
		{
			if (Rand_Num<degree[j])
			{
				degree_I=j+1;
				break;
			}
			degree_I=Ori_Pks_Num;
		}
		uint32_t *Neighbors_I=new uint32_t [degree_I];
		memset(Neighbors_I,0,sizeof(uint32_t)*degree_I);
		for (uint32_t Neighbors_count=0;Neighbors_count<degree_I;Neighbors_count++)
		{
			
			Neighbors_I[Neighbors_count]=floor(Ori_Pks_Num*(double)rand()/((double)RAND_MAX+1))+1;
			while(Repeat(Neighbors_I,Neighbors_count))
			{	
				Neighbors_I[Neighbors_count]=floor(Ori_Pks_Num*(double)rand()/((double)RAND_MAX+1))+1;
			}
			memcpy(Pks_read_I,buf+(Neighbors_I[Neighbors_count]-1)*Pks_Payload_Len,Pks_Payload_Len);
			if (!Neighbors_count)
			{
				memcpy(Pks_I+Pks_Header_Len,Pks_read_I,sizeof(char)*Pks_Payload_Len);
			}
			else
			{
				for (uint32_t l=0;l<Pks_Payload_Len;l++)
				{
					Pks_I[Pks_Header_Len+l]^=Pks_read_I[l];
				}
			}
			
		}
		memcpy(buf_send+Pks_Len*i,Pks_I,Pks_Len);
		delete []Neighbors_I;
	}
	delete []Pks_I;
	delete []Pks_read_I;
	//delete []Neighbors_I;
	
}

//Decoding Function
BOOL LT_Decode(char* buf,char* buf_Receive,DWORD m_dwFileSize,DWORD m_LTSendSize)
{
	char *Pks_D_I=new char [Pks_Len];
	memcpy(Pks_D_I,buf_Receive,Pks_Len);
	uint32_t filelen = *(uint32_t*)(Pks_D_I+2);
	uint32_t Ori_Pks_Num = ceil((double)filelen/(double)Pks_Payload_Len);
	uint32_t Enc_Pks_Num=m_LTSendSize/Pks_Len;
	char *Pks_buffer=new char[Pks_Payload_Len*Enc_Pks_Num];
	memset(Pks_buffer,0,sizeof(char)*Pks_Payload_Len*Enc_Pks_Num);

	BP_Index Ori_bp_index (Ori_Pks_Num);
	uint32_t *Ori_bp_process=new uint32_t[Ori_Pks_Num];
	BP_Index Enc_bp_index (Enc_Pks_Num);
	List Enc_bp_ripple;
	//initialing
	memset(Ori_bp_process,0,sizeof(uint32_t)*Ori_Pks_Num);

	char *Temp_pk1=new char[Pks_Payload_Len];
	char *Temp_pk2=new char[Pks_Payload_Len];
	uint32_t seed=0;
	uint32_t Ori_index=0;
	uint32_t Enc_index=0;

	double *degree=Degree(Ori_Pks_Num,0.01,0.5);
	uint32_t i;
	for (i=0;i<Enc_Pks_Num;i++)
	{	
		memcpy(Pks_D_I,buf_Receive+Pks_Len*i,Pks_Len);
		seed=*(uint32_t*)(Pks_D_I+6);
		memcpy(Pks_buffer+Pks_Payload_Len*i,Pks_D_I+Pks_Header_Len,Pks_Payload_Len);
		srand(seed);
		double Rand_D_Num=(double)rand()/((double)RAND_MAX+1);
		uint32_t degree_D_I=0;
		uint32_t j=0;
		for (j=0;j<Ori_Pks_Num;j++)
		{
			if (Rand_D_Num<degree[j])
			{
				degree_D_I=j+1;
				break;
			}
			degree_D_I=Ori_Pks_Num;
		}
		uint32_t *Neighbors_I=new uint32_t [degree_D_I];
		memset(Neighbors_I,0,sizeof(uint32_t)*degree_D_I);
		for (uint32_t Neighbors_count=0;Neighbors_count<degree_D_I;Neighbors_count++)
		{
			
			Neighbors_I[Neighbors_count]=floor(Ori_Pks_Num*(double)rand()/((double)RAND_MAX+1))+1;
			while(Repeat(Neighbors_I,Neighbors_count))
			{	
				Neighbors_I[Neighbors_count]=floor(Ori_Pks_Num*(double)rand()/((double)RAND_MAX+1))+1;
			}
			
		}
		for (uint32_t count_I=0;count_I<degree_D_I;count_I++)
		{
			Ori_index=Neighbors_I[count_I];
			Enc_bp_index.Ins_BP_Index((i+1),Ori_index);
			Ori_bp_index.Ins_BP_Index(Ori_index,i+1);
		}
		
	}

	for (i=0;i<Enc_Pks_Num;i++)
	{
		if (Enc_bp_index.m_degree[i]==1)
		{
			Enc_bp_ripple.Ins_list(i+1);
		}
	}

	while (Enc_bp_ripple.m_Len)
	{
		Enc_index=Enc_bp_ripple.Get_Del_head();
		Ori_index=Enc_bp_index.Get_Del_head(Enc_index);				
		Ori_bp_index.Del_BP_Index(Ori_index,Enc_index);				

		memcpy(buf+Pks_Payload_Len*(Ori_index-1),Pks_buffer+Pks_Payload_Len*(Enc_index-1),Pks_Payload_Len);
		Ori_bp_process[Ori_index-1]=1;
		if (sum(Ori_bp_process,Ori_Pks_Num)==Ori_Pks_Num)
		{
			break;
		}
		if (Ori_bp_index.m_degree[Ori_index-1])
		{
			while(Ori_bp_index.m_degree[Ori_index-1])
			{
				Enc_index=Ori_bp_index.Get_Del_head(Ori_index);
				Enc_bp_index.Del_BP_Index(Enc_index,Ori_index);
				memcpy(Temp_pk1,Pks_buffer+Pks_Payload_Len*(Enc_index-1),Pks_Payload_Len);
				memcpy(Temp_pk2,buf+Pks_Payload_Len*(Ori_index-1),Pks_Payload_Len);
				for (uint32_t l=0;l<Pks_Payload_Len;l++)
				{
					Temp_pk1[l]^=Temp_pk2[l];
				}	
				memcpy(Pks_buffer+Pks_Payload_Len*(Enc_index-1),Temp_pk1,Pks_Payload_Len);			
				if (Enc_bp_index.m_degree[Enc_index-1]==1)
				{
					Enc_bp_ripple.Ins_list(Enc_index);
				}
				if (Enc_bp_index.m_degree[Enc_index-1]==0)
				{
					Enc_bp_ripple.Del_list(Enc_index);
				}
			}
					
		}
				
	}

	
	//delete []Neighbors_I;	
		
	if (sum(Ori_bp_process,Ori_Pks_Num)==Ori_Pks_Num)
	{
		delete []Pks_D_I;
		delete []Pks_buffer;
		delete []Ori_bp_process;
		delete []Temp_pk1;
		delete []Temp_pk2;
		return true;
	}

	delete []Pks_D_I;
	delete []Pks_buffer;
	delete []Ori_bp_process;
	delete []Temp_pk1;
	delete []Temp_pk2;

	return false;
}
}

