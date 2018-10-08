


#include "table.h"
//#include "loader.h"
//#include <glob.h>
//#include <sys/mman.h>
//#include <sys/stat.h>
//#include <fcntl.h>
#include <iostream>
#include "stdlib.h"
//#include <assert.h>


void Table::init(Schema *s, unsigned int size)
{
    schema_ = s;
    data_head_ = 0;
    cur_ = 0;
}

LinkedTupleBuffer* Table:: get_root()
{
    return data_head_;
}

LinkedTupleBuffer* Table:: read_next()
{
    LinkedTupleBuffer *ret = cur_;
    if(cur_)
        cur_ = cur_->get_next();
    return ret;
}

void Table:: reset()
{
    cur_ = data_head_;
}

void Table:: close()
{
    LinkedTupleBuffer *t = data_head_;
    while(t)
    {
        t = data_head_->get_next();
        delete data_head_;
        data_head_ = t;
    }
    reset();
}

void WriteTable::init(Schema* s, unsigned int size)
{
    Table::init(s, size);
    this->size_ = size;
    data_head_ = new LinkedTupleBuffer(size, s->get_tuple_size());
    last_ = data_head_;
    cur_ = data_head_;
}

Table::LoadErrorT WriteTable::load(const string& filepattern,
        const string& separators)
{
    Loader loader(separators[0]);
    loader.load(filepattern, *this);
    return LOAD_OK;
}

void * WriteTable::append(const vector<string> &input)
{
    unsigned int s = schema_->get_tuple_size();
    if(! last_->can_store(s))
    {
        LinkedTupleBuffer *tmp = new LinkedTupleBuffer(size_, s);
        last_->set_next(tmp);
        last_ = tmp;
    }
    void *target = last_->allocate_tuple();
    schema_->parse_tuple(target, input);
    return target;
}

void WriteTable::append(const char **data, unsigned int count)
{
    unsigned int s = schema_->get_tuple_size();
    if(!last_->can_store(s))
    {
        LinkedTupleBuffer *tmp = new LinkedTupleBuffer(size_, s);
        last_->set_next(tmp);
        last_ = tmp;
    }
    if(schema_->columnCounts() == count)
    {
        void *target = last_->allocate_tuple();
        schema_->parse_tuple(target, data);
    }

    else
    {
        std::cout << "ERROR COUNT!" << endl;
        //throw NotYetImplemented();
    }
}

void WriteTable::append(const void * const src)
{
    unsigned int s = schema_->get_tuple_size();

    if(!last_->can_store(s))
    {
        LinkedTupleBuffer *tmp = new LinkedTupleBuffer(size_, s);
        last_->set_next(tmp);
        last_ = tmp;
    }

    void *target = last_->allocate_tuple();
    if(target != NULL)
    {
        schema_->copy_tuple(target, src);
    }
}

void WriteTable::concatenate(const WriteTable &table)
{
    if(schema_->get_tuple_size() == table.schema_->get_tuple_size())
    {
        if(data_head_ == 0)
        {
            data_head_ = table.data_head_;
        }
        last_->set_next(table.data_head_);
        last_ = table.last_;
    }
}
//typedef pair<string,string> CVpair; //colName+value;
void WriteTable::query(vector<CVpair>& clause, vector <void*>& tuple_res)
{
    for (int i = 0; i<clause.size(); i++)
    {
        if(!clause[i].first.compare("id"))
        {
            int id = atoi(clause[i].first.c_str());
            void *res = 0;
            if((res = query_index(id, clause)))
            {
                tuple_res.push_back(res);
            }
            break;
        }
        
        else
        {
            query_nonindex(clause, tuple_res);
        }
        
    }
}

void* WriteTable::query_index(int id_res, vector<CVpair>& clause)
{
    void *res;
    res = index_btree_->search(id_res);
    if(!res)
        return 0;
    for (int i = 0;i<clause.size();i++)
    {
        string colname = clause[i].first;
        unsigned int colIndex = schema_->getColPos(colname);
        vector<string> entry = schema_->output_tuple(res);
        if(entry[colIndex].compare(clause[i].second))
        {
            return 0;
        }
    }
    return res;
}
void WriteTable::query_nonindex(vector<CVpair>& clause, vector <void*>& tuple_res)
{
    vector<string>colName;
    vector<string>value;
    vector<int>colIndex;
    for(unsigned int i=0;i<clause.size();i++)
    {
        colName.push_back(clause[i].first);
        value.push_back(clause[i].second);
        colIndex.push_back(schema_->getColPos(colName[i]));
    }
    LinkedTupleBuffer* cur = data_head_;
    while(cur)
    {
        TupleBuffer::Iterator itr = cur->createIterator();
        void* tupleAddr = itr.next();
        while(tupleAddr)
        {
            if(!cur->empty_tuple(tupleAddr))
            {
                bool same = true;
                vector<string>entry = schema_->output_tuple(tupleAddr);
                for(unsigned int i=0;i<colName.size();i++)
                {
                    if(entry[colIndex[i]].compare(value[i]))
                    {
                        same = false;
                        break;
                    }
                }
                if(same)
                {
                    //int idIdex = schema_->getColPos("id");
                    tuple_res.push_back(tupleAddr);
                }
            }
            tupleAddr = itr.next();
        }
        cur = cur->get_next();
    }
}

void WriteTable::printTuples()
{
    LinkedTupleBuffer* cur = get_root();
    while(cur)
    {
        TupleBuffer::Iterator itr = cur->createIterator();
        void* tupelAddr = itr.next();
        while(tupelAddr)
        {
            if(!cur->empty_tuple(tupelAddr))
            {
                char sep = ' ';
                string tmp = schema()->pretty_print(tupelAddr,sep);
                cout << tmp.c_str() << endl;
            }
            tupelAddr = itr.next();
        }
        cur = cur->get_next();
    }
}

void WriteTable::printTuples(vector<void*>&input)
{
    char sep = ' ';
    for(unsigned int i=0;i<input.size();i++)
    {
        string line = schema()->pretty_print(input[i],sep);
        cout << line.c_str() <<endl;
    }
}

unsigned int WriteTable::tupleTotal()
{
    unsigned int ret = 0;
    LinkedTupleBuffer* cur = get_root();
    while(cur)
    {
        ret+= cur->getTupleCount();
        cur = cur ->get_next();
    }
    return ret;
}
bool WriteTable::deleteTuple(vector<CVpair>& clause)
{
    vector<void*>tuple_res;
    query(clause,tuple_res);
    if(!tuple_res.size())
    {
        cout << "Couldn't delete: no such tuple." <<endl;
        return false;
    }
    else
    {
        for(unsigned int i=0;i<tuple_res.size();i++)
        {
            int id_index = schema_->getColPos("id");
            string value_id = (schema_->output_tuple(tuple_res[i]))[id_index];
            LinkedTupleBuffer* p_cur = data_head_;
            LinkedTupleBuffer* p_pre = 0;
            while(p_cur)
            {
                TupleBuffer::Iterator itr= p_cur->createIterator();
                void* tupleAddr = itr.next();
                while(tupleAddr)
                {
                    if(!p_cur->empty_tuple(tupleAddr))
                    {
                        if(tupleAddr==tuple_res[i])
                        {
                            p_cur->delete_record(tupleAddr);
                            index_btree_->remove(atoi(value_id.c_str()));
                            double coord_x = *(double*)(schema()->calc_offset(tupleAddr,schema_->getColPos("x")));
                            double coord_y = *(double*)(schema()->calc_offset(tupleAddr, schema_->getColPos("y")));
                            struct data tmp(coord_x, coord_y);
                            KDpair del_pair = make_pair(tmp, tupleAddr);
                            index_kdtree_->delete_node(&del_pair);
                            break;
                        }
                    }
                    tupleAddr = itr.next();
                }
                if(p_cur->isEmptyBuffer())
                {
                    LinkedTupleBuffer* tmp = p_cur;
                    p_cur = p_cur->get_next();

                    if(tmp==data_head_)
                        data_head_ = p_cur;
                    else
                        p_pre->set_next(p_cur);
                    delete tmp;
                }
                else
                {
                    p_pre = p_cur;
                    p_cur = p_cur->get_next();
                }
            }
        }
        return true;
    }
}
void* WriteTable::search_tuple(string id)
{
    int i = atoi(id.c_str());
    return index_btree_->search(i);
}
/*void* WriteTable::search_tuple(string id)
{
    LinkedTupleBuffer* cur = get_root();
    while(cur)
    {
        TupleBuffer::Iterator itr = cur->createIterator();
        void* tupleAddr = itr.next();
        while(tupleAddr)
        {
            if(!cur->empty_tuple(tupleAddr))
            {
                vector<string> entry = schema()->output_tuple(tupleAddr);
                string cmpId = entry[schema()->getColPos("id")];
                if(!cmpId.compare(id))
                    return tupleAddr;
            }
            tupleAddr = itr.next();
        }
        cur = cur->get_next();
    }
    return 0;
}*/

bool WriteTable::insert(vector<CVpair>& entry)
{
    if(schema()->columnCounts()!=entry.size())
    {
        cout << "insert error: the column count is different from schema." << endl;
        return false;
    }
    string id ="";
    for(unsigned int i=0;i<entry.size();i++)
    {
        if(!(entry[i].first).compare("id"))
        {
            id = entry[i].second;
            break;
        }
    }
    if(!id.compare(""))
    {
        cout << "insert error: no col is named \'id\'" << endl;
        return false;
    }
    if(search_tuple(id))
    {
        cout << "insert error: id is repeated."<<endl;
        return false;
    }
    vector<string> input;
    for(unsigned int i=0;i<entry.size();i++)
        input.push_back("");
    double coord_x;
    double coord_y;
    for(unsigned int i=0;i<entry.size();i++)
    {
        int index = schema()->getColPos(entry[i].first);
        if(index<0)
        {
            cout << "insert error: can't find the col named "  << entry[i].first.c_str() <<endl;
            return false;
        }

        if(!entry[i].first.compare("x"))
            coord_x = atof(entry[i].second.c_str());
        if(!entry[i].second.compare("y"))
            coord_y = atof(entry[i].second.c_str());
        input[index] = entry[i].second;
    }
    void* target = append(input);
    /*index insert*/
    KVpair insert_KV(atoi(id.c_str()), target);
    index_btree_->insert(insert_KV);
    index_kdtree_->insert(coord_x, coord_y, target);

    return true;
}

vector<void*> WriteTable::RangeQuery(double centerX, double centerY, double r)
{
    vector<KDpair *> res_pair;
    vector<void*> res;
    struct data *center = new struct data(centerX, centerY);
    res_pair = index_kdtree_->RangeQuery(center, r);
    for(int i=0;i<res_pair.size();i++)
        res.push_back(res_pair[i]->second);
    return res;

}

/*double WriteTable::distance(double x,double y,double centerX,double centerY)
{
    return sqrt(pow(x-centerX,2)+pow(y-centerY,2));
}

vector<void*> WriteTable::RangeQuery(double centerX,double centerY,double r)
{
    vector<void*>ret;
    LinkedTupleBuffer* cur = get_root();
    while(cur)
    {
        TupleBuffer::Iterator itr = cur->createIterator();
        void* tupleAddr  = itr.next();
        while(tupleAddr)
        {
            if(!cur->empty_tuple(tupleAddr))
            {
                Schema* s = schema();
                vector<string>entry = s->output_tuple(tupleAddr);
                unsigned int xIndex = s->getColPos("x");
                unsigned int yIndex = s->getColPos("y");
                istringstream isX(entry[xIndex]);
                istringstream isY(entry[yIndex]);
                double x,y;
                isX>>x;
                isY>>y;
                if(distance(x,y,centerX,centerY)<=r)
                    ret.push_back(tupleAddr);
            }
            tupleAddr = itr.next();
        }
        cur = cur->get_next();
    }
    return ret;
}*/

bool WriteTable::update(vector<CVpair>& clause,vector<CVpair>& newCV)
{
    vector<void*>tuple_res;
    query(clause,tuple_res);
    Schema* s =schema();
    if(!tuple_res.size())
    {
        cout << "update error: no such tuple." <<endl;
        return false;
    }
    int idIndex = -1;
    string id = "id";
    for(unsigned int i=0;i<newCV.size();i++)
    {
        if(!id.compare(newCV[i].first))
        {
            idIndex = i;
            break;
        }
    }
    if(idIndex>=0)
    {
        if(tuple_res.size()>1)
        {
            cout << "update error: can't update the id of more than one tuple."<<endl;
            return false;
        }
        if(void* tmp_addr = search_tuple(newCV[idIndex].second))
        {

            if(tuple_res[0] != tmp_addr)
            {
                cout << "update error: the new value of id is existed." <<endl;
                return false;  
            }

        }
    }

    vector<string>value;
    vector<int>colIndex;
    int coord_x_index = -1;
    int coord_y_index = -1;
    for(unsigned int i=0;i<newCV.size();i++)
    {
        string colName = newCV[i].first;
        colIndex.push_back(s->getColPos(colName));
        value.push_back(newCV[i].second);
        if(colIndex[i] == schema()->getColPos("x"))
            coord_x_index = i;
        if(colIndex[i] == schema()->getColPos("y"))
            coord_y_index = i;

    }
    double coord_x_value;
    double coord_y_value;
    if(coord_x_index>0){
        coord_x_value = atof(value[coord_x_index].c_str());        
    }
    if(coord_y_index>0)
        coord_y_value = atof(value[coord_y_index].c_str());
    for(unsigned int i=0;i<tuple_res.size();i++)
    {
        void* dest = tuple_res[i];
        for(unsigned int j=0;j<value.size();j++)
        {
            parse_updated_data(dest,(char*)(value[j].c_str()),colIndex[j]);
        }
        if(coord_x_index<0&&coord_y_index>0)
            coord_x_value = *(double *)schema()->calc_offset(tuple_res[i], schema()->getColPos("x"));
        else if(coord_y_index<0&&coord_x_index>0)
            coord_y_value = *(double *)schema()->calc_offset(tuple_res[i], schema()->getColPos("x"));
        else if(coord_y_index<0&&coord_x_index<0)
            continue;
        struct data tmp(coord_x_value, coord_y_value);
        KDpair update_pair = make_pair(tmp, tuple_res[i]);
        index_kdtree_->update_coord(&update_pair);
    }
    return true;
}

void WriteTable::parse_updated_data(void *dest, char *input, int col)
{
    ColumnType type = schema_->get_column_type(col);
    switch(type)
    {
        case CT_INTEGER:
            int val;
            val = atoi(input);
            schema_->write_data(dest, col, &val);
            break;
        case CT_LONG:
            long long val3;
            val3 = atoll(input);
            schema_->write_data(dest, col, &val3);
            break;
        case CT_DECIMAL:
            double val2;
            val2 = atof(input);
            schema_->write_data(dest, col, &val2);
            break;
        case CT_CHAR:
            schema_->write_data(dest, col, input);
            break;
        case CT_POINTER:
            cout<<"pointer cannot be transferred!"<<endl;
            break;
    }
}

void WriteTable::setModify()
{
    isModify_ = true;
}

void WriteTable::setUnmodify()
{
    isModify_ = false;
}

bool WriteTable::isModify()
{
    return isModify_;
}

void WriteTable::Try_rdlock()
{
    pthread_rwlock_rdlock(&lock_);
}

void WriteTable::Try_wrlock()
{
    pthread_rwlock_wrlock(&lock_);
}

void WriteTable::unlock()
{
    pthread_rwlock_unlock(&lock_);
}





