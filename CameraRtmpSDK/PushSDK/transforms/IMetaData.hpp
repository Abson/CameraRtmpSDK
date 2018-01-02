//
//  IMetaData.hpp
//  CameraRtmpSDK
//
//  Created by Abson on 2016/11/23.
//  Copyright © 2016年 Abson. All rights reserved.
//

#ifndef IMetaData_hpp
#define IMetaData_hpp

#include <iostream>

struct IMetadata
{};

template <int32_t MetaDataType, typename... Types>
struct MetaData : IMetadata {


    MetaData<Types...>() {};

    const int32_t type() const { return MetaDataType; }

    void setData(Types... data)
    {
        m_data = std::make_tuple(data...);
    }

    template<std::size_t idx>
    using Tupple_ElementType = typename std::tuple_element<idx, std::tuple<Types...>>::type;

    template<std::size_t idx>
    void setValue(Tupple_ElementType<idx> value)
    {
        auto& v = getData<idx>();
        v = value;
    }

    template<std::size_t idx>
    Tupple_ElementType<idx>& getData() const {
        return const_cast<Tupple_ElementType<idx>&>(std::get<idx>(m_data));
    }

    size_t size() const {

    }

private:
    std::tuple<Types...> m_data;
};

namespace push_sdk {

}

#endif /* IMetaData_hpp */
